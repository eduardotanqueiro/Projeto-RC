#include <regex.h>



int fd_server, porto;
struct sockaddr_in addr_connect,addr_receive_updates;
char available_markets[NUMBER_MARKETS][MAX_STRING_SIZES];

pthread_mutex_t check_feed = PTHREAD_MUTEX_INITIALIZER;
int feed_on;

int socket_multicast,addrlen;
struct sockaddr_in addr_multicast;


//
int login();
void ReceiveMarketsInfo();
void* HandleStocksUpdates(void* ip_multicast);
void TalkWithServer();
void BuyOrSellStock(int mode);
void TrySubscribeMarket();
void ChangeStateFeed();
void AskWalletStatus();

void start_socket();
void erro(char *s);