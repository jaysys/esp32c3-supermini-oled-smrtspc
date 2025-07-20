//
// 동작시킨후에 시리얼 창에서 "table" 커맨드를 보내면 구성내역을 보여준다.
//
// 

#include <WiFi.h>
#include <Preferences.h>
#include <EEPROM.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <map>
#include <numeric>

#define EEPROM_SIZE 512  // EEPROM size
#include <esp_partition.h> // ESP-IDF 파티션 관련 API 헤더
// #include <esp_flash.h>  // Not needed if using ESP.getFlashChipSize()
#include <esp_ota_ops.h>   // For esp_ota_get_running_partition()
#include <ESP.h>           // For ESP.getFlashChipSize()

/**
 * @brief 현재 ESP32 장치의 플래시 파티션 정보를 조회하여 출력합니다.
 * 주로 4MB 플래시 보드에서 사용되는 파티션 스키마를 가정하고 있습니다.
 */
void printFlashPartitionInfo() {
  Serial.println("\n--- ESP32 Flash Partition Info ---");

  // 플래시 칩 크기 조회
  // CORRECTED: Use ESP.getFlashChipSize()
  uint32_t flash_chip_size = ESP.getFlashChipSize();
  Serial.printf("Flash Chip Size: %u MB\n", flash_chip_size / (1024 * 1024));
  Serial.println("------------------------------------");

  // 모든 파티션을 찾기 위한 이터레이터
  esp_partition_iterator_t it;

  // 'app' (애플리케이션) 타입 파티션 조회
  it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (it) {
    Serial.println("### Application Partitions (APP) ###");
    do {
      const esp_partition_t* part = esp_partition_get(it);
      Serial.printf("  Label: %-15s, Type: %2d, SubType: %2d, Address: 0x%06x, Size: %6u KB\n",
                    part->label,
                    part->type,
                    part->subtype,
                    part->address,
                    part->size / 1024);
      it = esp_partition_next(it);
    } while (it != NULL);
    esp_partition_iterator_release(it); // 이터레이터 해제
  } else {
    Serial.println("  No APP partitions found.");
  }
  Serial.println("------------------------------------");

  // 'data' 타입 파티션 조회 (NVS, OTA Data, SPIFFS/LittleFS 등)
  it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (it) {
    Serial.println("### Data Partitions ###");
    do {
      const esp_partition_t* part = esp_partition_get(it);
      // 서브타입에 따라 더 구체적인 이름 출력
      const char* subTypeStr;
      if (part->subtype == ESP_PARTITION_SUBTYPE_DATA_NVS) {
        subTypeStr = "NVS";
      } else if (part->subtype == ESP_PARTITION_SUBTYPE_DATA_OTA) {
        subTypeStr = "OTA_DATA";
      } else if (part->subtype == ESP_PARTITION_SUBTYPE_DATA_PHY) {
        subTypeStr = "PHY_INIT";
      } else if (part->subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS) {
        subTypeStr = "SPIFFS";
      } else if (part->subtype == ESP_PARTITION_SUBTYPE_DATA_FAT) {
        subTypeStr = "FATFS";
      } else {
        subTypeStr = "UNKNOWN";
      }

      Serial.printf("  Label: %-15s, Type: %2d, SubType: %-10s, Address: 0x%06x, Size: %6u KB\n",
                    part->label,
                    part->type,
                    subTypeStr, // 문자열 서브타입 사용
                    part->address,
                    part->size / 1024);
      it = esp_partition_next(it);
    } while (it != NULL);
    esp_partition_iterator_release(it); // 이터레이터 해제
  } else {
    Serial.println("  No Data partitions found.");
  }
  Serial.println("------------------------------------");

  // 현재 실행 중인 애플리케이션 파티션 정보
  const esp_partition_t* running_partition = esp_ota_get_running_partition();
  if (running_partition) {
    Serial.printf("Currently Running App: %s (Address: 0x%06x)\n",
                  running_partition->label, running_partition->address);
  }
  Serial.println("------------------------------------");
}
  
 

void initNVS() {
  esp_err_t ret = nvs_flash_init();
  
  // If NVS is not initialized or corrupted
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND || 
      ret == ESP_ERR_NVS_NOT_FOUND || ret == ESP_ERR_NVS_INVALID_STATE) {
    
    Serial.println("⚠️ NVS partition not found or corrupted. Erasing...");
    
    // Erase NVS partition
    if (nvs_flash_erase() != ESP_OK) {
      Serial.println("❌ Failed to erase NVS partition");
      return;
    }
    
    // Initialize NVS again
    ret = nvs_flash_init();
    if (ret != ESP_OK) {
      Serial.printf("❌ Failed to initialize NVS: %s\n", esp_err_to_name(ret));
      return;
    }
    
    Serial.println("✅ NVS partition erased and reinitialized");
  } else if (ret != ESP_OK) {
    Serial.printf("❌ Failed to initialize NVS: %s\n", esp_err_to_name(ret));
    return;
  }
  
  Serial.println("✅ NVS initialized successfully");
}

void testNVS() {
  Serial.println("\n🧪 Testing NVS...");
  
  // Test writing and reading a value
  nvs_handle_t my_handle;
  esp_err_t err;
  
  // Open NVS
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    Serial.printf("❌ Error opening NVS: %s\n", esp_err_to_name(err));
    return;
  }
  
  // Try to read a test value
  int32_t test_value = 0;
  err = nvs_get_i32(my_handle, "test_key", &test_value);
  
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    Serial.println("ℹ️ No test value found, writing one...");
    test_value = 1234;
    err = nvs_set_i32(my_handle, "test_key", test_value);
    if (err == ESP_OK) {
      err = nvs_commit(my_handle);
      if (err == ESP_OK) {
        Serial.println("✅ Test value written successfully");
      }
    }
  } else if (err == ESP_OK) {
    Serial.printf("✅ Read test value: %d\n", test_value);
  }
  
  nvs_close(my_handle);
}

void listAllNVSContents() {
  // Initialize Serial if not already done
  if (!Serial) {
    Serial.begin(115200);
    while (!Serial) { ; }
  }
  
  Serial.println("\n📋 NVS 전체 내용 나열:");
  Serial.println("========================");
  
  // Maps for tracking entries per namespace
  std::map<String, int> namespace_entries;
  std::map<String, int> actual_entry_counts;
  
  // Initialize NVS iterator
  nvs_iterator_t it = NULL;
  esp_err_t res = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY, &it);
  
  if (res != ESP_OK) {
    Serial.println("❌ NVS 엔트리를 찾을 수 없습니다.");
    return;
  }
  
  // First pass: collect all namespaces
  while (res == ESP_OK) {
    nvs_entry_info_t info;
    nvs_entry_info(it, &info);
    String ns_name = String(info.namespace_name);
    namespace_entries[ns_name] = 0;
    actual_entry_counts[ns_name] = 0;
    res = nvs_entry_next(&it);
  }
  nvs_release_iterator(it);
  
  // Process each namespace
  size_t namespace_index = 0;
  for (auto& ns : namespace_entries) {
    const char* namespace_name = ns.first.c_str();
    int entry_count = 0;
    
    Serial.printf("\n📁 네임스페이스: %s\n", namespace_name);
    
    // Find all entries in this namespace
    nvs_iterator_t ns_it = NULL;
    esp_err_t ns_res = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY, &ns_it);
    
    while (ns_res == ESP_OK) {
      nvs_entry_info_t info;
      nvs_entry_info(ns_it, &info);
      
      // Only process entries in the current namespace
      if (strcmp(namespace_name, info.namespace_name) != 0) {
        ns_res = nvs_entry_next(&ns_it);
        continue;
      }
      
      // Open the namespace
      nvs_handle_t handle;
      if (nvs_open(info.namespace_name, NVS_READONLY, &handle) != ESP_OK) {
        Serial.printf("  ❌ Failed to open namespace: %s\n", info.namespace_name);
        ns_res = nvs_entry_next(&ns_it);
        continue;
      }
      
      // Count and display the entry
      actual_entry_counts[String(info.namespace_name)]++;
      entry_count++;
      
      // Display the entry based on its type
      esp_err_t err = ESP_OK;
      switch (info.type) {
        case NVS_TYPE_I8: {
          int8_t val;
          err = nvs_get_i8(handle, info.key, &val);
          if (err == ESP_OK) Serial.printf("  🔑 %-20s | 타입: int8   | 값: %d\n", info.key, val);
          break;
        }
        case NVS_TYPE_U8: {
          uint8_t val;
          err = nvs_get_u8(handle, info.key, &val);
          if (err == ESP_OK) Serial.printf("  🔑 %-20s | 타입: uint8  | 값: %u\n", info.key, val);
          break;
        }
        case NVS_TYPE_I16: {
          int16_t val;
          err = nvs_get_i16(handle, info.key, &val);
          if (err == ESP_OK) Serial.printf("  🔑 %-20s | 타입: int16  | 값: %d\n", info.key, val);
          break;
        }
        case NVS_TYPE_U16: {
          uint16_t val;
          err = nvs_get_u16(handle, info.key, &val);
          if (err == ESP_OK) Serial.printf("  🔑 %-20s | 타입: uint16 | 값: %u\n", info.key, val);
          break;
        }
        case NVS_TYPE_I32: {
          int32_t val;
          err = nvs_get_i32(handle, info.key, &val);
          if (err == ESP_OK) Serial.printf("  🔑 %-20s | 타입: int32  | 값: %d\n", info.key, val);
          break;
        }
        case NVS_TYPE_U32: {
          uint32_t val;
          err = nvs_get_u32(handle, info.key, &val);
          if (err == ESP_OK) Serial.printf("  🔑 %-20s | 타입: uint32 | 값: %u\n", info.key, val);
          break;
        }
        case NVS_TYPE_STR: {
          size_t len;
          err = nvs_get_str(handle, info.key, NULL, &len);
          if (err == ESP_OK) {
            char* str = (char*)malloc(len);
            if (str) {
              nvs_get_str(handle, info.key, str, &len);
              Serial.printf("  🔑 %-20s | 타입: string | 값: %s\n", info.key, str);
              free(str);
            }
          }
          break;
        }
        case NVS_TYPE_BLOB: {
          size_t len;
          err = nvs_get_blob(handle, info.key, NULL, &len);
          if (err == ESP_OK) Serial.printf("  🔑 %-20s | 타입: blob   | 크기: %d bytes\n", info.key, len);
          break;
        }
        default:
          Serial.printf("  🔑 %-20s | 알 수 없는 타입: %d\n", info.key, info.type);
      }
      
      if (err != ESP_OK) {
        Serial.printf("  ❌ %-20s | 읽기 실패: %s\n", info.key, esp_err_to_name(err));
      }
      
      nvs_close(handle);
      ns_res = nvs_entry_next(&ns_it);
    }
    // Clean up the iterator
    if (ns_it) {
      nvs_release_iterator(ns_it);
      ns_it = NULL;
    }
    
    // Print entry count for this namespace
    Serial.printf("  └── 총 %d개 엔트리\n", entry_count);
    
    // Print separator if not the last namespace
    if (++namespace_index < namespace_entries.size()) {
      Serial.println("------------------------");
    }
  }
  
  // Print summary
  Serial.println("\n📊 네임스페이스 요약:");
  int total_entries = 0;
  for (auto const& ns : actual_entry_counts) {
    Serial.printf("  - %-15s: %d개 엔트리\n", ns.first.c_str(), ns.second);
    total_entries += ns.second;
  }
  
  Serial.printf("\n✅ 총 %d개의 네임스페이스, %d개의 엔트리가 조회되었습니다.\n", 
               (int)actual_entry_counts.size(), total_entries);
  Serial.println("========================");
}

void dumpNVS() {
  Serial.println("\n NVS ");
  Serial.println("======================");
  
  // Initialize NVS
  initNVS();
  
  // Get NVS statistics
  nvs_stats_t nvs_stats;
  if (nvs_get_stats(NULL, &nvs_stats) == ESP_OK) {
    Serial.println("📊 NVS 파티션 통계:");
    Serial.printf("  사용 중인 엔트리: %d\n", nvs_stats.used_entries);
    Serial.printf("  사용 가능한 엔트리: %d\n", nvs_stats.free_entries);
    Serial.printf("  총 엔트리: %d\n", nvs_stats.total_entries);
    Serial.printf("  네임스페이스 수: %d\n", nvs_stats.namespace_count);
  } else {
    Serial.println("❌ NVS 통계를 가져올 수 없습니다.");
  }
  
  // 모든 NVS 내용 나열
  listAllNVSContents();
}

void deleteNamespace(const char* namespace_name) {
  if (namespace_name == NULL || strlen(namespace_name) == 0) {
    Serial.println("❌ 잘못된 네임스페이스 이름입니다.");
    return;
  }
  
  // NVS 초기화 확인
  initNVS();
  
  // 네임스페이스 존재 여부 확인
  nvs_iterator_t it = NULL;
  bool namespace_exists = false;
  
  esp_err_t res = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY, &it);
  while (res == ESP_OK) {
    nvs_entry_info_t info;
    nvs_entry_info(it, &info);
    
    if (strcmp(namespace_name, info.namespace_name) == 0) {
      namespace_exists = true;
      break;
    }
    res = nvs_entry_next(&it);
  }
  nvs_release_iterator(it);
  
  if (!namespace_exists) {
    Serial.printf("❌ '%s' 네임스페이스를 찾을 수 없습니다.\n", namespace_name);
    return;
  }
  
  // 네임스페이스 삭제
  nvs_handle_t handle;
  if (nvs_open(namespace_name, NVS_READWRITE, &handle) != ESP_OK) {
    Serial.println("❌ 네임스페이스를 열 수 없습니다.");
    return;
  }
  
  // 모든 키 삭제
  if (nvs_erase_all(handle) != ESP_OK) {
    Serial.println("❌ 네임스페이스 삭제에 실패했습니다.");
    nvs_close(handle);
    return;
  }
  
  // 변경사항 커밋
  if (nvs_commit(handle) != ESP_OK) {
    Serial.println("❌ 변경사항을 저장하는데 실패했습니다.");
    nvs_close(handle);
    return;
  }
  
  nvs_close(handle);
  Serial.printf("✅ '%s' 네임스페이스가 성공적으로 삭제되었습니다.\n", namespace_name);
}

void dumpEEPROM() {
  Serial.println("\n📋 Dumping EEPROM contents:");
  Serial.println("==========================");
  
  EEPROM.begin(EEPROM_SIZE);
  
  for (int i = 0; i < EEPROM_SIZE; i += 16) {
    if (i % 128 == 0) {
      Serial.println();
      Serial.printf("0x%04X: ", i);
    }
    
    for (int j = 0; j < 16; j++) {
      if (i + j < EEPROM_SIZE) {
        byte value = EEPROM.read(i + j);
        if (value < 0x10) Serial.print('0');
        Serial.print(value, HEX);
        Serial.print(' ');
      }
    }
    
    Serial.print(" ");
    for (int j = 0; j < 16 && (i + j) < EEPROM_SIZE; j++) {
      byte value = EEPROM.read(i + j);
      if (value >= 32 && value <= 126) {
        Serial.write(value);
      } else {
        Serial.print('.');
      }
    }
    Serial.println();
  }
  
  EEPROM.end();
  Serial.println("\n✅ EEPROM dump completed");
  Serial.println("==========================");
}

void showNVSValue(String keyPath) {
  int firstDotIndex = keyPath.indexOf('.');
  if (firstDotIndex == -1) {
    Serial.println("❌ 잘못된 형식입니다. '네임스페이스.키' 형식으로 입력해주세요 (예: phy.cal_mac)");
    Serial.println("⚠️  참고: nvs.net80211.* 네임스페이스는 보안상 직접 접근이 제한될 수 있습니다.");
    return;
  }

  String namespaceName = keyPath.substring(0, firstDotIndex);
  String keyName = keyPath.substring(firstDotIndex + 1);
  
  nvs_handle_t handle;
  esp_err_t err;
  
  // For PHY and other standard namespaces
  if (namespaceName == "phy" || namespaceName == "nvs") {
    // Try opening with default partition first
    err = nvs_open(namespaceName.c_str(), NVS_READONLY, &handle);
    
    // If that fails, try with explicit partition
    if (err != ESP_OK) {
      err = nvs_open_from_partition("nvs", namespaceName.c_str(), NVS_READONLY, &handle);
    }
    
    // Special case for WiFi settings (just show a helpful message)
    if (namespaceName == "nvs" && keyName.startsWith("net80211")) {
      Serial.println("⚠️  WiFi 설정(nvs.net80211.*)은 보안상 직접 접근이 제한됩니다.");
      Serial.println("    대신 WiFi.SSID(), WiFi.BSSIDstr() 등의 API를 사용해주세요.");
      return;
    }
  } 
  // For other namespaces
  else {
    err = nvs_open(namespaceName.c_str(), NVS_READONLY, &handle);
  }
  if (err != ESP_OK) {
    // Only show error if this is a direct user request (not during scanning)
    Serial.printf("❌ 네임스페이스 '%s'을(를) 찾을 수 없습니다.\n", namespaceName.c_str());
    return;
  }

  // Try to determine the key type by attempting to read with different types
  esp_err_t read_err;
  
  // Try reading as string first
  size_t str_len;
  read_err = nvs_get_str(handle, keyName.c_str(), NULL, &str_len);
  if (read_err == ESP_OK) {
    char* str = (char*)malloc(str_len);
    if (str) {
      read_err = nvs_get_str(handle, keyName.c_str(), str, &str_len);
      if (read_err == ESP_OK) {
        Serial.printf("📌 %s.%s (string): %s\n", namespaceName.c_str(), keyName.c_str(), str);
      }
      free(str);
      nvs_close(handle);
      return;
    }
  }
  
  // Try reading as blob
  size_t blob_len;
  read_err = nvs_get_blob(handle, keyName.c_str(), NULL, &blob_len);
  if (read_err == ESP_OK) {
    uint8_t* blob = (uint8_t*)malloc(blob_len);
    if (blob) {
      read_err = nvs_get_blob(handle, keyName.c_str(), blob, &blob_len);
      if (read_err == ESP_OK) {
        Serial.printf("📌 %s.%s (blob, %d bytes): ", namespaceName.c_str(), keyName.c_str(), blob_len);
        for (size_t i = 0; i < blob_len && i < 32; i++) {
          Serial.printf("%02X ", blob[i]);
        }
        if (blob_len > 32) Serial.print("...");
        Serial.println();
      }
      free(blob);
      nvs_close(handle);
      return;
    }
  }
  
  // Try reading as different numeric types
  int8_t i8_val;
  read_err = nvs_get_i8(handle, keyName.c_str(), &i8_val);
  if (read_err == ESP_OK) {
    Serial.printf("📌 %s.%s (int8): %d\n", namespaceName.c_str(), keyName.c_str(), i8_val);
    nvs_close(handle);
    return;
  }
  
  uint8_t u8_val;
  read_err = nvs_get_u8(handle, keyName.c_str(), &u8_val);
  if (read_err == ESP_OK) {
    Serial.printf("📌 %s.%s (uint8): %u\n", namespaceName.c_str(), keyName.c_str(), u8_val);
    nvs_close(handle);
    return;
  }
  
  int16_t i16_val;
  read_err = nvs_get_i16(handle, keyName.c_str(), &i16_val);
  if (read_err == ESP_OK) {
    Serial.printf("📌 %s.%s (int16): %d\n", namespaceName.c_str(), keyName.c_str(), i16_val);
    nvs_close(handle);
    return;
  }
  
  uint16_t u16_val;
  read_err = nvs_get_u16(handle, keyName.c_str(), &u16_val);
  if (read_err == ESP_OK) {
    Serial.printf("📌 %s.%s (uint16): %u\n", namespaceName.c_str(), keyName.c_str(), u16_val);
    nvs_close(handle);
    return;
  }
  
  int32_t i32_val;
  read_err = nvs_get_i32(handle, keyName.c_str(), &i32_val);
  if (read_err == ESP_OK) {
    Serial.printf("📌 %s.%s (int32): %d\n", namespaceName.c_str(), keyName.c_str(), i32_val);
    nvs_close(handle);
    return;
  }
  
  uint32_t u32_val;
  read_err = nvs_get_u32(handle, keyName.c_str(), &u32_val);
  if (read_err == ESP_OK) {
    Serial.printf("📌 %s.%s (uint32): %u\n", namespaceName.c_str(), keyName.c_str(), u32_val);
    nvs_close(handle);
    return;
  }
  
  // If we get here, we couldn't determine the key type
  Serial.printf("❌ 키 '%s'의 타입을 확인할 수 없거나 존재하지 않습니다.\n", keyName.c_str());
  nvs_close(handle);
  return;

}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "dump nvs") {
      dumpNVS();
    } 
    else if (command == "dump eeprom") {
      dumpEEPROM();
    }
    else if (command.startsWith("delete ")) {
      String ns_to_delete = command.substring(7);
      if (ns_to_delete.length() > 0) {
        deleteNamespace(ns_to_delete.c_str());
      } else {
        Serial.println("❌ 삭제할 네임스페이스 이름을 입력해주세요. 예: delete wifi_config");
      }
    }
    else if (command.startsWith("show ")) {
      String keyPath = command.substring(5);
      if (keyPath.length() > 0) {
        showNVSValue(keyPath);
      } else {
        Serial.println("❌ 조회할 키를 입력해주세요. 예: show nvs.net80211.sta.ssid");
      }
    }
    else if (command.startsWith("table")){
      printFlashPartitionInfo();
    }
    else if (command == "help") {
      Serial.println("\n사용 가능한 명령어:");
      Serial.println("  dump nvs          - NVS 내용 조회");
      Serial.println("  dump eeprom       - EEPROM 내용 조회");
      Serial.println("  delete <네임스페이스> - 특정 네임스페이스 삭제");
      Serial.println("  show <키경로>     - 특정 키의 값 조회 (예: show nvs.net80211.sta.ssid)");
      Serial.println("  help              - 도움말 보기");
    }
    else if (command != "") {
      Serial.println("❌ Unknown command. Type 'help' for available commands.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize
  
  // Initialize NVS first
  initNVS();
  
  // Initialize EEPROM with the defined size
  EEPROM.begin(EEPROM_SIZE);
  
  Serial.println("\n🔧 System started. Type 'help' for available commands.");
  
  WiFi.persistent(false);             // 자동 저장 막기
  WiFi.disconnect(true, true);        // 이전 정보 제거
  delay(200);

  WiFi.mode(WIFI_STA);                // 명시적 모드 설정
  WiFi.begin("123456", "123456"); // 직접 설정

  Serial.print("📶 연결 중");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ 연결 성공");
  } else {
    Serial.println("\n❌ 연결 실패");
  }
}

void loop() {
  handleSerialCommands();
  delay(100);
}


