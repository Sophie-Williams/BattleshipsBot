// BattleshipBot.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include "math.h"

#pragma comment(lib, "wsock32.lib")


#define STUDENT_NUMBER		"13014222"
#define STUDENT_FIRSTNAME	"Adam"
#define STUDENT_FAMILYNAME	"Matheson"

#define IP_ADDRESS_SERVER	"127.0.0.1"
//#define IP_ADDRESS_SERVER "164.11.80.55"

#define PORT_SEND	 1924 // We define a port that we are going to use.
#define PORT_RECEIVE 1925 // We define a port that we are going to use.


#define MAX_BUFFER_SIZE	500
#define MAX_SHIPS		200

#define FIRING_RANGE	100

#define MOVE_LEFT		-1
#define MOVE_RIGHT		 1
#define MOVE_UP			 1
#define MOVE_DOWN		-1
#define MOVE_FAST		 2
#define MOVE_SLOW		 1


SOCKADDR_IN sendto_addr;
SOCKADDR_IN receive_addr;

SOCKET sock_send;  // This is our socket, it is the handle to the IO address to read/write packets
SOCKET sock_recv;  // This is our socket, it is the handle to the IO address to read/write packets

WSADATA data;

char InputBuffer [MAX_BUFFER_SIZE];

// SETUP VARIABLES
// About my ship
int myX; // My X location
int myY; // My Y location
int myHealth; // Green = 10 - 9, Yellow = 7 - 8, Red = 0 - 1 etc.
int myFlag; // My flag value

// About all ships (Index 0 is always my ship)
int number_of_ships; // How many ships are in visible range
int shipX[MAX_SHIPS]; // X locations of the ships within visible range
int shipY[MAX_SHIPS]; // Y locations of the ships within visible range
int shipHealth[MAX_SHIPS]; // Describes health of the ships within visible range
int shipFlag[MAX_SHIPS]; // Describes the flag of the ships within visible range

// Other variables
bool fire = false;
int fireX;
int fireY;

bool moveShip = false;
int moveX;
int moveY;

bool setFlag = true;
int new_flag = 4222;

void fire_at_ship(int X, int Y);
void move_in_direction(int left_right, int up_down);
void set_new_flag(int newFlag);

/*************************************************************/
/********************* Potential Tactics *********************/
/*************************************************************/

//1.) Remain in centre/corner of map, defensively shooting down passers by, no matter health or number. 
// Not too effective, your ship's movement is limited and because all ships are of equal speed, it cannot simply poach ships as they pass by as I had hoped. SCRAPPED 18/02/14

//2.) Move around entire map, aggressively shooting down any other ship, advanced movements may be necessary.
// Works ok on bots server but may encounter problems when facing human players with very advanced tactics, that can outmaneuver/outshoot my ship. NEEDS MORE TESTING 01/03/14

//3.) Move around entire map, shooting down ships if they are closest and/or fewer in number. Move towards weakest ship.
// Works well in most situations, simple. Could be expanded. Testing yielded good results and improvements. CURENT WORKING VERSION 04/04/14

//4.) Move around entire map, defensively running from other ships unless there is only one damaged one in range.
// NOT TESTED 23/03/14

// TACTIC 3 CHOSEN AS FINAL TACTIC. OTHERS CANNED. 04/04/14

/*************************************************************/
/********* Your tactics code starts here *********************/
/*************************************************************/

// Declare necessary variables and structures for tactics() to execute correctly
int up_down = MOVE_LEFT*MOVE_SLOW;
int left_right = MOVE_UP*MOVE_FAST;
int target;

int i;
int x;
int y;
int r;
int min_health;
int closest;
int shipDistance[MAX_SHIPS];

int number_of_friends;
int friendX[MAX_SHIPS];
int friendY[MAX_SHIPS];
int friendHealth[MAX_SHIPS];
int friendFlag[MAX_SHIPS];
int friendDistance[MAX_SHIPS];

int number_of_enemies;
int enemyX[MAX_SHIPS];
int enemyY[MAX_SHIPS];
int enemyHealth[MAX_SHIPS];
int enemyFlag[MAX_SHIPS];
int enemyDistance[MAX_SHIPS];

// NOTE:
// The reason for putting this code in a function was to collect what was a very simple piece of code into one group, and therefore make it easier to read in the context of tactics().
// It also demonstrates I am able to write functions.
// I felt that other code in tatics() could be put into a function, but it was not necessary to do. It is already fairly short, tidy, readable and well commented.
// Putting the code in a function would have next to no effect on efficiency due to the speed of the computers. Therefore readability is more important.
// Collecting all the code together under the single tactics() function created better readability, in my opinion.
void return_to_battle() // Code to re-direct my ship if it is heading out of the battlefield limits
{
	if (myY > 700) // Remain within a square in the middle - the rest of my alliance will do the same, creating a central "black hole" swallowing other ships, rather than spreading our strength thin.
	{
		up_down = MOVE_DOWN*MOVE_SLOW;
	}
	if (myX < 300)
	{
		left_right = MOVE_RIGHT*MOVE_FAST;
	}
	if (myY < 300)
	{
		up_down = MOVE_UP*MOVE_FAST;
	}
	if (myX > 700)
	{
		left_right = MOVE_LEFT*MOVE_SLOW;
	}
}

// NOTE:
// The following encrypt() function was not used as it was not working 100% on the day.
// It is left here for marking purposes only and was REMOVED during the simulation to improve efficiency.
void encrypt() // Code to encrypt my flag
{
	CONST int Flag = 4222; // Create a localised variable for my flag, separate from the standard flag variable so it is easy to switch between the two if necessary.
	int cryptbase1 = myX, cryptbase2 = myY; // Encrypted flag is based on my location, so it is impossible for it to be copied and an enemy to pretend they are me!
	int encrypted = (Flag + cryptbase1) ^ cryptbase2;
	int decrypted= ( encrypted ^ cryptbase2) - cryptbase1;
	printf("Decrypted: %d\n", decrypted); // Display the decrypted and encrypted flags, for debugging purposes
	new_flag = encrypted;
	printf("Encrypted: %d\n", encrypted);
}

bool IsAFriend(int index) // Code to return true if the visible ship has a decrypted flag in my alliance.
{
	bool rc; // Setup localised variables
	rc = false;

	int cryptbase1 =  shipX[index], cryptbase2= shipY[index]; // encryptedFlag is ships original flag
	int encryptedFlag = shipFlag[index]; // Calculate decrypted flag (encryptedFlag and shipY) - shipX
	int decryptedFlag = (encryptedFlag ^ cryptbase2) - cryptbase1;

	// Enter all friends flags here
	if (decryptedFlag == 29763) { //Jake
		rc = true;
	}
	if (decryptedFlag == 6969) { // Dan
		rc = true;
	}
	if (decryptedFlag == 9001) { // Edwin
		rc = true;
	}
	if (decryptedFlag == 1600) { // Aiden
		rc = true;
	}
	if (decryptedFlag == 13303) { // Ollie
		rc = true;
	}
	if (decryptedFlag == 4212) { // James
		rc = true;
	}
	if (decryptedFlag == 8653) { // Tianyi
		rc = true;
	}
	if (decryptedFlag == 13243) { // Anab
		rc = true;
	}
	if (decryptedFlag == 16342) { // Shaz
		rc = true;
	}
	return rc;
}

void tactics() // Code to target the closest enemy in range and move towards the weakest visible enemy
{
	return_to_battle();
	// encrypt(); This is where the encrypt function would have gone, had it been included on the day (see encrypt() function comments for more details).

	if (number_of_ships > 1) // If there are multiple ships, then this code decides which to target and which to move towards
	{
		number_of_friends = 0; // Setup localised variables
		number_of_enemies = 0;

		for (i = 1; i < number_of_ships; i++) // This equation calculates distance of all ships using pythagoras theorem
		{
			x = myX - shipX[i];
			y = myY - shipY[i];
			r = (x*x) + (y*y);
			shipDistance[i] = (int)sqrt((double)r);
		}

		for (i = 1; i < number_of_ships; i++) // Iterate through all visible ships
		{
			if(IsAFriend(i)) // If this visible ship is a friend...
			{
				friendX[number_of_friends] = shipX[i]; // Set the player up as such
				friendY[number_of_friends] = shipY[i];
				friendHealth[number_of_friends] = shipHealth[i];
				friendFlag[number_of_friends] = shipFlag[i];
				friendDistance[number_of_friends] = shipDistance[i];
				number_of_friends++; // Increase the number of visible friends on record
			}
			else // But if he is a foe...
			{
				enemyX[number_of_enemies] = shipX[i]; // Set the player up as such
				enemyY[number_of_enemies] = shipY[i];
				enemyHealth[number_of_enemies] = shipHealth[i];
				enemyFlag[number_of_enemies] = shipFlag[i];
				enemyDistance[number_of_enemies] = shipDistance[i];
				number_of_enemies++; // Increase the number of visible enemies on record
			}
		}

		enemyDistance[number_of_enemies] = shipDistance[i];

		if (number_of_enemies > 0) // If enemies exist...
		{
			min_health = 0; // Setup localised variables
			closest = 0;

			for (i = 0; i < number_of_enemies; i++) // Iterate through all those enemies
			{
				if (enemyDistance[closest] > enemyDistance[i]) // Find closest enemy
				{
					closest = i;
				}
				if (myHealth > enemyHealth[i]) // Find weakest enemy
				{
					min_health = i;
				}
			}

			fire_at_ship(enemyX[closest], enemyY[closest]); // Fire at closest enemy

			if (myX < enemyX[min_health]) // Set heading towards weakest enemy
			{
				left_right = MOVE_RIGHT*MOVE_FAST;
			}
			else
			{
				left_right = MOVE_LEFT*MOVE_FAST;
			}
			if (myY < enemyY[min_health])
			{
				up_down = MOVE_UP*MOVE_FAST;
			}
			else
			{
				up_down = MOVE_DOWN*MOVE_FAST;
			}
		}
	}
	move_in_direction(left_right, up_down); // Move on previously calculated heading
}

/*************************************************************/
/********* Your tactics code ends here ***********************/
/*************************************************************/

void communicate_with_server()
{
	char buffer[4096];
	int  len = sizeof(SOCKADDR);
	char chr;
	bool finished;
	int  i;
	int  j;
	int  rc;
	char* p;

	sprintf_s(buffer, "Register  %s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME);
	sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));

	while (true)
	{
		if (recvfrom(sock_recv, buffer, sizeof(buffer)-1, 0, (SOCKADDR *)&receive_addr, &len) != SOCKET_ERROR)
		{
			p = ::inet_ntoa(receive_addr.sin_addr);
			if ((strcmp(IP_ADDRESS_SERVER, "127.0.0.1") == 0) || (strcmp(IP_ADDRESS_SERVER, p) ==0))
			{
				i = 0;
				j = 0;
				finished = false;
				number_of_ships = 0;

				while ((!finished) && (i<4096))
				{
					chr = buffer[i];

					switch (chr)
					{
					case '|':
						InputBuffer[j] = '\0';
						j = 0;
						sscanf_s(InputBuffer,"%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships]);
						number_of_ships++;
						break;

					case '\0':
						InputBuffer[j] = '\0';
						sscanf_s(InputBuffer,"%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships]);
						number_of_ships++;
						finished = true;
						break;

					default:
						InputBuffer[j] = chr;
						j++;
						break;
					}
					i++;
				}
			}

			myX = shipX[0];
			myY = shipY[0];
			myHealth = shipHealth[0];
			myFlag = shipFlag[0];


			tactics();

			if (fire)
			{
				sprintf_s(buffer, "Fire %s,%d,%d", STUDENT_NUMBER, fireX, fireY);
				sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
				fire = false;
			}

			if (moveShip)
			{
				sprintf_s(buffer, "Move %s,%d,%d", STUDENT_NUMBER, moveX, moveY);
				rc = sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
				moveShip = false;
			}

			if (setFlag)
			{
				sprintf_s(buffer, "Flag %s,%d", STUDENT_NUMBER, new_flag);
				sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
				setFlag = false;
			}

		}
		else
		{
			printf_s("recvfrom error = %d\n", WSAGetLastError());
		}
	}

	printf_s("Student %s\n", STUDENT_NUMBER);
}


void fire_at_ship(int X, int Y)
{
	fire = true;
	fireX = X;
	fireY = Y;
}


void move_in_direction(int X, int Y)
{
	if (X < -2) X = -2;
	if (X >  2) X =  2;
	if (Y < -2) Y = -2;
	if (Y >  2) Y =  2;

	moveShip = true;
	moveX = X;
	moveY = Y;
}


void set_new_flag(int newFlag)
{
	setFlag = true;
	new_flag = newFlag;
}


int _tmain(int argc, _TCHAR* argv[])
{
	char chr = '\0';

	printf("\n");
	printf("Battleship Bots\n");
	printf("UWE Computer and Network Systems Assignment 2 (2013-14)\n");
	printf("\n");

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return(0);

	//sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	//if (!sock)
	//{	
	//	printf("Socket creation failed!\n"); 
	//}

	sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_send)
	{	
		printf("Socket creation failed!\n"); 
	}

	sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_recv)
	{	
		printf("Socket creation failed!\n"); 
	}

	memset(&sendto_addr, 0, sizeof(SOCKADDR_IN));
	sendto_addr.sin_family = AF_INET;
	sendto_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	sendto_addr.sin_port = htons(PORT_SEND);

	memset(&receive_addr, 0, sizeof(SOCKADDR_IN));
	receive_addr.sin_family = AF_INET;
	//	receive_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	receive_addr.sin_addr.s_addr = INADDR_ANY;
	receive_addr.sin_port = htons(PORT_RECEIVE);

	int ret = bind(sock_recv, (SOCKADDR *)&receive_addr, sizeof(SOCKADDR));
	//	int ret = bind(sock_send, (SOCKADDR *)&receive_addr, sizeof(SOCKADDR));
	if (ret)
	{
		printf("Bind failed! %d\n", WSAGetLastError());  
	}

	communicate_with_server();

	closesocket(sock_send);
	closesocket(sock_recv);
	WSACleanup();

	while (chr != '\n')
	{
		chr = getchar();
	}

	return 0;
}

