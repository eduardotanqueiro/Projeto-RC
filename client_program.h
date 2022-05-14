#include <regex.h>


int fd_server, porto;
struct sockaddr_in addr_connect,addr_receive_updates;
char available_markets[NUMBER_MARKETS][MAX_STRING_SIZES];

pthread_mutex_t check_feed;
int feed_on;


//
int login();
int ReceiveMarketsInfo();
void* HandleStocksUpdates(void* ip_multicast);
void TalkWithServer();
void BuyOrSellStock(int mode);
void TrySubscribeMarket();
void ChangeStateFeed();
int check_regex(char *text, char *regex);