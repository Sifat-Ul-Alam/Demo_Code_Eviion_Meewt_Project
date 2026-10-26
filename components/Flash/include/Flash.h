#include <stdio.h>

#define NVS_NAMESPACE    "storage"
#define NVS_KEY_UINT64_1 "uint64_value1"
#define NVS_KEY_UINT64_2 "uint64_value2"
#define NVS_KEY_UINT64_3 "uint64_value3"
#define NVS_KEY_UINT64_4 "uint64_value4"
#define NVS_KEY_TODO_REMINDERS "todo_reminders"   /* saved to-do reminder table (see raven_operations.c) */
#define NVS_KEY_WATER_REMINDER "water_reminder"   /* saved water-reminder setting (see raven_operations.c) */
#define NVS_KEY_DOB_REMINDER   "dob_reminder"     /* saved birthday (month + day, see raven_operations.c) */

void     Save_Uint64_To_Flash(uint64_t value, const char* key);
uint64_t Read_Uint64_From_Flash(const char* key);

void     Save_Blob_To_Flash(const char* key, const void* data, size_t len);
int      Read_Blob_From_Flash(const char* key, void* data, size_t len);   /* 1 = read, 0 = nothing saved (or size differs) */

