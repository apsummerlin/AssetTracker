
#ifndef commands_h
#define commands_h

#define MAX_FILE_SIZE 10 * 1024

void send_file(void);
void send_small_file(void);

bool myBlockLookup(unsigned long long id, char data[128]);

void delete_directory(void);
void make_directory(void);
void change_directory(void);

void time(void);

void gps_status(void);

#endif