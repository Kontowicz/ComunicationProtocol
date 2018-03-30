#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <random>
#include <sstream>
#include <complex>
#include <conio.h>
#include <map>
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27015
#define IPADDR "0.0.0.0"


char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

float silnia(float a)
{
	float doZwrotu = 1;
	for (int i = 1; i <= a; i++)
	{
		doZwrotu *= i;
	}
	return doZwrotu;
}

float log(float a, float b)
{
	float doZwrotu = 0;

	doZwrotu = ((log10(b)) / (log10(a)));

	return  doZwrotu;
}

std::string floatToString(float liczba)
{
	std::stringstream floatToStr;
	std::string str;
	floatToStr << liczba;
	floatToStr >> str;
	floatToStr.clear();
	return str;
}

std::string intToString(int a)
{
	std::string s;
	sprintf((char*)s.c_str(), "%d", a);
	return s.c_str();
}

void wyslij(SOCKET&ConnectSocket, std::string pakiet)
{
	int kod;
	kod = send(ConnectSocket, pakiet.c_str(), pakiet.length(), 0);
	if (kod == SOCKET_ERROR) {
		std::clog<<"Send blad:"<< WSAGetLastError()<<"\n";
		closesocket(ConnectSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
} 

std::string odbierz(SOCKET&ConnectSocket)
{
	int kod;
	std::string dane = "";
	kod = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
	if (kod > 0) {
		dane = "";
		for (int i = 0; i < kod; i++)
		{
			dane += recvbuf[i];
		}
	}
	else if (kod == 0)
		std::clog << "Polaczenie zamkniete.\n";
	else
		std::clog << "Recv blad:" << WSAGetLastError() << "\n";

	return dane;
}

int generujID()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 500);
	return dis(gen);
}

#pragma region operacje na pakietach
std::string stworzPakiet(std::string operacja, std::string status, std::string id, std::string argument1, std::string argument2)
{
	auto czas = std::chrono::system_clock::now();
	time_t czasAktualny = std::chrono::system_clock::to_time_t(czas);
	std::string czasS = ctime(&czasAktualny);
	czasS.erase(czasS.end() - 1);

	std::string pakiet = "";

	pakiet = "#1:C=" + czasS + "+#2:O=" + operacja + "+#3:S=" + status
		+ "+#4:I=" + id + "+#5:A=" + argument1 + "+#6:B=" + argument2;

	return pakiet;
}

std::array<std::string, 6> dekodujPakiet(std::string pakiet)
{
	/*doZwrotu[0] - czas
	doZwrotu[1] - operacja
	doZwrotu[2] - status
	doZwrotu[3] - id
	doZwrotu[4] - argument 1
	doZwrotu[5] - argument 2*/

	std::array<std::string, 6> doZwrotu;

	int a = pakiet.find("#1:C=") + 5;
	int b = pakiet.find("+#2:O=");

	doZwrotu[0] = pakiet.substr(a, b - a); // czas

	a = pakiet.find("+#2:O=") + 6;
	b = pakiet.find("+#3:S=");
	doZwrotu[1] = pakiet.substr(a, b - a); // operacja

	a = pakiet.find("+#3:S=") + 6;
	b = pakiet.find("+#4:I=");
	doZwrotu[2] = pakiet.substr(a, b - a).c_str(); // status

	a = pakiet.find("+#4:I=") + 6;
	b = pakiet.find("+#5:A=");
	doZwrotu[3] = pakiet.substr(a, b - a); // id

	a = pakiet.find("+#5:A=") + 6;
	b = pakiet.find("+#6:B=");
	doZwrotu[4] = pakiet.substr(a, b - a); // argument a

	a = pakiet.find("+#6:B=") + 6;
	b = pakiet.length();
	doZwrotu[5] = pakiet.substr(a, b - a); // argument b

	return doZwrotu;
}
#pragma endregion operacje na pakietach

int main()
{
	std::map<std::string, int> operacje;
	operacje["ID"] = 0;
	operacje["SUMA"] = 1;
	operacje["ROZNICA"] = 2;
	operacje["MNOZENIE"] = 3;
	operacje["DZIELENIE"] = 4;
	operacje["POTEGOWANIE"] = 5;
	operacje["PIERWIASTKOWANIE"] = 6;
	operacje["LOGARYTM"] = 7;
	operacje["SUMAKWADRATOWLICZB"] = 8;
	operacje["SILNIA"] = 9;
	operacje["ZAKONCZ"] = 11;
	operacje["BLAD"] = 12;
	operacje["ODP"] = 13;
	// Zmienne s³u¿ace do obs³ugi po³¹czeñ
	WSADATA wsaData;
	int kod;
	int iSendResult;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	struct sockaddr_in serwer;

	// Zmienne pomocnicze s³u¿ace do obs³ugi operacji
	int idSesji = 0;
	std::string pakiet = "";
	std::array<std::string, 6> dekodowane;

	kod = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (kod != 0) {
		std::clog << "WSASturtup blad: " << kod << "\n";
		exit(EXIT_FAILURE);
	}

	serwer.sin_family = AF_INET;
	InetPton(AF_INET, IPADDR, &serwer.sin_addr);
	serwer.sin_port = htons(DEFAULT_PORT);

	// Tworzenie socketu do ³¹czenia siê z serwerem
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		std::clog << "socket zanczone bledem: " << WSAGetLastError() << "\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// Ustawienia nas³uchuj¹cego socketu
	kod = bind(ListenSocket, reinterpret_cast<sockaddr*>(&serwer), sizeof(serwer));
	if (kod == SOCKET_ERROR) {
		std::clog << "bind zakoczone bledem: " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}


	// Nas³uchiwanie 
	kod = listen(ListenSocket, SOMAXCONN);
	if (kod == SOCKET_ERROR) {
		std::clog << "listen zakocznone bledem: " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// Akceptacja po³¹czenia
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		std::clog << "accept zakonczone bledem: " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// Nie potrzeba nas³uchiwaæ nadchodz¹cych po³¹czeñ
	closesocket(ListenSocket);

	// Ustalenie ip sesji
	pakiet = odbierz(ClientSocket);
	dekodowane = dekodujPakiet(pakiet);
	if (operacje[dekodowane[1]] == 0) {
		std::clog << "Prosze o id.\n";
		idSesji = generujID();
		std::clog << "id: " << idSesji << "\n";
		pakiet = stworzPakiet("ODP", "OK", intToString(idSesji), "0", "0");
		wyslij(ClientSocket, pakiet);
	}


	// Ods³uga/komunikacja 
	do {
		pakiet = odbierz(ClientSocket);
		dekodowane = dekodujPakiet(pakiet);

		switch (operacje[dekodowane[1]])
		{
		case 1:
		{
			std::clog << "Dodawanie liczb " << dekodowane[4] << " i " << dekodowane[5] << "\n";
			float a, b;
			a = atof(dekodowane[4].c_str());
			b = atof(dekodowane[5].c_str());
			std::string wynik = floatToString(a + b);
			if (wynik == "inf") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 2:
		{
			std::clog << "Odejmowanie liczb " << dekodowane[4] << " i " << dekodowane[5] << "\n";
			float a, b;
			a = atof(dekodowane[4].c_str());
			b = atof(dekodowane[5].c_str());
			std::string wynik = floatToString(a - b);
			if (wynik == "inf") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 3:
		{
			std::clog << "Mnozenie liczb " << dekodowane[4] << " i " << dekodowane[5] << "\n";
			float a, b;
			a = atof(dekodowane[4].c_str());
			b = atof(dekodowane[5].c_str());
			std::string wynik = floatToString(a * b);
			if (wynik == "inf") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 4:
		{
			std::clog << "Dzielenie liczb " << dekodowane[4] << " i " << dekodowane[5] << "\n";
			float a, b;
			a = atof(dekodowane[4].c_str());
			b = atof(dekodowane[5].c_str());
			std::string wynik = floatToString(a / b);
			if (wynik == "inf") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 5:
		{
			std::clog << "Potegowanie liczb " << dekodowane[4] << " i " << dekodowane[5] << "\n";
			float a, b;
			a = atof(dekodowane[4].c_str());
			b = atof(dekodowane[5].c_str());
			std::string wynik = floatToString(std::pow(a, b));
			if (wynik == "inf") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 6:
		{
			std::clog << "Pierwiastkowanie " << dekodowane[5] << " i " << dekodowane[4] << "\n";
			float a, b;
			a = atof(dekodowane[5].c_str());
			b = atof(dekodowane[4].c_str());
			std::string wynik = floatToString(std::pow(b, (1 / a)));
			if (wynik == "inf") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 7:
		{
			std::clog << "Logarytm " << dekodowane[4] << " i " << dekodowane[5] << "\n";
			float a, b;
			a = atof(dekodowane[4].c_str());
			b = atof(dekodowane[5].c_str());
			std::string wynik = floatToString(log(a, b));
			if (wynik == "inf" || wynik == "-nan(ind)" || wynik == "-0") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 8:
		{
			std::clog << "Suma kwadratow liczb " << dekodowane[4] << " i " << dekodowane[5] << "\n";
			float a, b;
			a = atof(dekodowane[4].c_str());
			b = atof(dekodowane[5].c_str());

			std::string wynik = floatToString(std::pow(a, 2) + std::pow(b, 2));
			if (wynik == "inf") {
				pakiet = stworzPakiet("ODP", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 9:
		{
			std::clog << "Silnia " << dekodowane[4] << "\n";
			float liczba = atof(dekodowane[4].c_str());

			std::string wynik = floatToString((silnia(liczba)));
			if (wynik == "inf") {
				pakiet = stworzPakiet("BLAD", "BLAD", intToString(idSesji), "0", "0");
			}
			else {
				pakiet = stworzPakiet("ODP", wynik, intToString(idSesji), "0", "0");
			}
			wyslij(ClientSocket, pakiet);
		}
		break;
		case 11:
		{
			// Zakoñcz
			pakiet = stworzPakiet("ZAKONCZ", "OK", intToString(idSesji), "0", "0");
			wyslij(ClientSocket, pakiet);
			// Zakoñczenie po³¹czenia
			kod = shutdown(ClientSocket, SD_SEND);
			if (kod == SOCKET_ERROR) {
				std::clog << "shutdown zakonczone bledem: " << WSAGetLastError() << "\n";
				closesocket(ClientSocket);
				WSACleanup();
				exit(EXIT_FAILURE);
			}
			std::clog << "Koncze polaczanie\n";
			closesocket(ClientSocket);
			WSACleanup();
			_getch();
			exit(EXIT_SUCCESS);

		}
		break;
		}
	} while (atoi(dekodowane[1].c_str()) != 11);
	exit(EXIT_SUCCESS);
}