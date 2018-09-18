#include <array>
#include <blepp/blestatemachine.h> 
#include <blepp/lescan.h>
#include <blepp/logging.h>
#include <blepp/pretty_printers.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <boost/optional.hpp>
#include <cerrno>
#include <ctime> 
#include <iomanip>
#include <map>
#include <signal.h>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <thd/aes.hpp>
#include <unistd.h>
#include <vector>

using namespace std;
using namespace BLEPP;

void catch_function(int)
{
  cerr << "\nInterrupted!\n";
}

void print_time() {
  struct timeval tv;
  time_t nowtime;
  struct tm *nowtm;
  char tmbuf[64], buf[64];

  gettimeofday(&tv, NULL);
  nowtime = tv.tv_sec;
  nowtm = localtime(&nowtime);
  strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
  snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv.tv_usec);
  cout << buf;
}

int main(int argc, char** argv)
{

  HCIScanner::ScanType type = HCIScanner::ScanType::Active;
  HCIScanner::FilterDuplicates filter = HCIScanner::FilterDuplicates::Software;
  int c;
  string help = R"X(-[h]:
  -h  show this message
)X";

  filter = HCIScanner::FilterDuplicates::Off;

  opterr = 0;

  string key_s;

  while((c=getopt(argc, argv, "!k:h")) != -1)
  {
    switch(c) {
    case 'k':
      key_s = string(optarg);
      break;
    case 'h':
      cout << "Usage: " << argv[0] << " " << help;
      return 0;
    case '?':
      if (optopt == 'k') {
	cerr << "Option k requires an argument (key)" << endl;
      }
      return 1;
    default:
      cerr << argv[0] << ":  unknown option " << c << endl;
      return 1;
    }
  }
  if (key_s.size() != 32) {
    cout << "Key should be present and have 16 digits (size = " << key_s.size() << ")" << endl;
    return 1;
  }
  uint8_t key[16], key_s0, key_s1;
  for (int i = 0; i < 32; i+=2) { 
    if (key_s[i] > 0x60) {
      key_s0 = key_s[i] - 0x61 + 10;
    } else {
      key_s0 = key_s[i] - 0x30;
    }
    if (key_s[i+1] > 0x60) {
      key_s1 = key_s[i+1] - 0x61 + 10;
    } else {
      key_s1 = key_s[i+1] - 0x30;
    }
    key[i/2] = (key_s0 << 4) + key_s1;
    //cout << to_hex(key[i]);
  } 
  //cout << endl;

  log_level = LogLevels::Error;
  HCIScanner *scanner;
  try {
    scanner = new HCIScanner(true, filter, type);
  } catch(BLEPP::HCIScanner::HCIError) {
    cerr << "Does the device actually exist?" << endl;
    cerr << "Try something like this (reload the kernel module, unblock via rfkill, bring up interface):" << endl;
    cerr << "  sudo rmmod btusb" << endl;
    cerr << "  sudo modprobe btusb" << endl;
    cerr << "  sudo rfkill unblock bluetooth" << endl;
    cerr << "  sudo hciconfig hci0 up" << endl;
    exit(1);
  } catch(BLEPP::HCIScanner::IOError) {
    cerr << "You probably can solve this by using sudo." << endl;
    cerr << "  sudo ./crownstone" << endl;
    cerr << "You can also give the binary these so-called \"capabilities\" to be able to scan (and run as normal user)" << endl;
    cerr << "  sudo setcap cap_net_raw+ep crownstone" << endl;
    cerr << "  ./crownstone" << endl;
    exit(2);
  }

  //Catch the interrupt signal. If the scanner is not 
  //cleaned up properly, then it doesn't reset the HCI state.
  signal(SIGINT, catch_function);

  //Something to print to demonstrate the timeout.
  string throbber="/|\\-";

  //hide cursor, to make the throbber look nicer.
  cout << "[?25l" << flush;

  bool print_packet = false;
  bool print_all_scan_responses = false;

  int i=0;
  while (1) {

    //Check to see if there's anything to read from the HCI
    //and wait if there's not.
    struct timeval timeout;     
    timeout.tv_sec = 0;     
    timeout.tv_usec = 300000;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(scanner->get_fd(), &fds);
    int err = select(scanner->get_fd()+1, &fds, NULL, NULL,  &timeout);

    //Interrupted, so quit and clean up properly.
    if(err < 0 && errno == EINTR) {
      break;
    }

    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);                                                                                            

    bool parse_advertisements = false;
	      
    uint8_t encrypted_data[16];

    if(FD_ISSET(scanner->get_fd(), &fds))
    {
      //Only read id there's something to read
      vector<AdvertisingResponse> ads = scanner->get_advertisements();

      for(const auto& ad: ads)
      {
	if ((ad.type == LeAdvertisingEventType::ADV_IND) || (ad.type == LeAdvertisingEventType::ADV_NONCONN_IND)
	    || (ad.type == LeAdvertisingEventType::ADV_SCAN_IND) || (ad.type == LeAdvertisingEventType::ADV_DIRECT_IND)) {
	  if (parse_advertisements && ad.manufacturer_specific_data.size() > 0) {
	    for(const auto& data: ad.manufacturer_specific_data) {
	      // decode only iBeacon messages
	      if (data.size() == 25) {
		cout << ad.address << "\t";
		cout << '[' << to_hex((uint8_t)data[3]) << "] ";
		cout << " uuid: ";
		for (int i = 4; i < 8; ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << '-';
		for (int i = 8; i < 10; ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << '-';
		for (int i = 10; i < 12; ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << '-';
		for (int i = 12; i < 14; ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << '-';
		for (int i = 14; i < 20; ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << " major: 0x";
		for (int i = 20; i < 22; ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << " minor: 0x";
		for (int i = 22; i < 24; ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << " tx power: 0x";
		for (int i = 24; i < data.size(); ++i) {
		  cout << to_hex((uint8_t)data[i]);
		}
		cout << endl;
	      } 
	    }
	  }
	} 
	// ad.type == LeAdvertisingEventType::SCAN_RSP
	else {
	  bool success = false;
	  if (ad.unparsed_data_with_types.size() > 0) {
	    uint8_t data_type, device_type, length;
	    for(const auto& data: ad.unparsed_data_with_types) {
	      length = data[3];
	      // note the endianness here!
	      data_type = (data[4] >> 4) & 0x0F;
	      device_type = data[4] & 0x0F;
	      memcpy(encrypted_data, &data[5], 16);
	      AES_ECB_decrypt(&ctx, encrypted_data);
	      if (encrypted_data[15] == 0xFA) {
		success = true;
	      }
	    }
	    if (print_all_scan_responses || success) {
	      cout << ad.address << " ";
	      cout << '[' << to_hex(length) << "] ";
	      switch(device_type) {
	      case 1: 
		cout << "plug";
		break;
	      case 2:
		cout << "guidestone";
		break;
	      case 3:
		cout << "built-in";
		break;
	      case 4:
		cout << "dongle";
		break;
	      default:
		cout << "unknown [" << to_hex(device_type) << ']';
	      }
	      cout << ' ';
	      switch(data_type) {
	      case 0:
		cout << "state";
		break;
	      case 1: 
		cout << "error";
		break;
	      case 2:
		cout << "external state";
		break;
	      case 3:
		cout << "external error";
		break;
	      default:
		cout << "unknown [" << to_hex(data_type) << ']';
	      }
	      /* raw decrypted data
	      cout << " ";
	      for (int i = 0; i < 16; ++i) {
		cout << to_hex((uint8_t)encrypted_data[i]);
	      } */
	      
	      uint8_t crownstone_id, switch_state, flags, temperature, reserved, validation;
	      crownstone_id = encrypted_data[1];
	      cout << " id " << (int)crownstone_id;
	      switch_state = encrypted_data[2];
	      cout << " switch state " << (int)switch_state;
	      flags = encrypted_data[3];
	      cout << " flags " << (int)flags;
	      temperature = encrypted_data[4];
	      cout << " temperature " << (int)temperature;

	      float power_factor = (float)(encrypted_data[5])/127.0;
	      cout << " power factor " << power_factor;

	      uint8_t power_usage0, power_usage1;
	      power_usage0 = encrypted_data[6];
	      power_usage1 = encrypted_data[7];

	      uint16_t power_usage = (uint16_t)(power_usage1 << 8) + power_usage0;
	      cout << " power[W] " << (float)power_usage / 8.0;
	      cout << " power[VA] " << (float)power_usage / (8.0 * power_factor);

	      uint8_t energy_used0, energy_used1, energy_used2, energy_used3;
	      energy_used0 = encrypted_data[8];
	      energy_used1 = encrypted_data[9];
	      energy_used2 = encrypted_data[10];
	      energy_used3 = encrypted_data[11];

	      uint32_t energy_used = (uint32_t)(energy_used3 << 24) + (uint32_t)(energy_used2 << 16) + (uint32_t)(energy_used1 << 8) + energy_used0;
	      cout << " energy_used " << energy_used;
	      
	      uint8_t partial_timestamp0, partial_timestamp1;
	      partial_timestamp0 = encrypted_data[12];
	      partial_timestamp1 = encrypted_data[13];
	      uint16_t partial_timestamp = (uint16_t)(partial_timestamp1 << 8) + partial_timestamp0;

	      cout << " time " << (int)partial_timestamp;
	      
	      reserved = encrypted_data[14];
	      cout << " reserved " << (int)reserved;

	      validation = encrypted_data[15];
	      cout << " validation 0x" << to_hex(validation);
	      cout << endl;


	    }
	  }
	}
      }
    }
    else
      cout << throbber[i%4] << "\b" << flush;
    i++;
  }

  //show cursor
  cout << "[?25h" << flush;
}
