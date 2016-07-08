/*
 *main.cpp
 *https://123a321.wordpress.com/2010/02/01/serial-port-with-mingw/
 *https://github.com/waynix/SPinGW
 *https://github.com/waynix/SPinGW/blob/master/example.c
 *Threads - https://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
 *Created on: Jun 21, 2016
 *Author: Deepa
*/
// init_log.txt - log response from ICount on initializing commands.
// init_file.txt -  command file for initializing ICount.
// IPD_log - log-file for the program.
//
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "serialport.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <map>
#include <list>
#include <string>

using namespace std;
string action;
bool started = false;
bool log_open = false;

DWORD WINAPI readICount(LPVOID lpParameter);
DWORD WINAPI userEntry(LPVOID lpParameter);

int analyzeData();
int initializeICount(HANDLE h);

ofstream IPD_log;

int main(void)
{
	IPD_log<<"Main"<<endl;
	HANDLE  hReadICount;
	HANDLE 	hReadUser;
	HANDLE	hSerialPort;

	DWORD ICount;
	DWORD User;

	IPD_log.open("IPD_log.txt");

	/*
	cout<<"Enter port number : ";
	cin>>port_num;

	if (port_num.length()>4)
		port_num= "\\\\.\\" + port_num;

	cout<<"Port num"<<port_num<<endl;

	hSerialPort = openSerialPort(port_num.c_str(),B9600,one,off); */

	hSerialPort = openSerialPort("COM3",B9600,one,off);

	if(!initializeICount(hSerialPort))
	{
		IPD_log<<"Error: IPD initialization failed."<<endl;
		return -1;
	}

	hReadICount = CreateThread(
					0,               		// default security attributes
					0,                    	// use default stack size
					readICount,    	 	 	// thread function name
					hSerialPort,       		// argument to thread function
					0,                    	// use default creation flags
					&ICount); 				// returns the thread identifier

	hReadUser = CreateThread(
						0,               		// default security attributes
						0,                    	// use default stack size
						userEntry,    	 	 	// thread function name
						0,       				// argument to thread function
						0,                    	// use default creation flags
						&User); 				// returns the thread identifier

	WaitForSingleObject(hReadICount, INFINITE);
	WaitForSingleObject(hReadUser, INFINITE);

	CloseHandle(hReadICount);

	closeSerialPort(hSerialPort);
	IPD_log<<"Port closed."<<endl; // to be removed

	if(!analyzeData())
	{
		IPD_log<<"Error: Data analysis failed ."<<endl;
		return -1;
	}
	cout<<"End of program.";

	IPD_log.close();
	return 0;

}

int initializeICount(HANDLE h)
{
	IPD_log<<"Initialize IPD"<<endl;
	int bytesWritten;
	int bytesRead;

	char readbuffer[250];

	string sendbuffer;
	string initlog;

	ifstream init_file("init_file.txt");
	ofstream init_log("init_log.txt");

	Sleep(5000);

	if(init_file.is_open())
	{
		if (init_log.is_open())
		{
			if ( init_file.peek() == std::ifstream::traits_type::eof() )
			{
				IPD_log<<"Error: init_file.txt is empty."<<endl;
				return 0;
			}
			while(getline(init_file,sendbuffer))
			{
				char *buffer = new char[sendbuffer.length()+2];
				strcpy(buffer,sendbuffer.c_str());

				buffer[sendbuffer.length()] ='\r';
				buffer[sendbuffer.length()+1] ='\0';

				bytesWritten = writeToSerialPort(h,buffer,strlen(buffer));
				Sleep(100);
				int n =0;
				while(n<5)
				{
					bytesRead	 = readFromSerialPort(h,readbuffer,99);
					readbuffer[bytesRead] ='\0';
					if(bytesRead>0)
					init_log<<readbuffer<<endl;
					Sleep(100);
					n++;
				}
			}
		}
		init_log.close();
	}
	else
	{
		IPD_log<<"Error: init_file.txt cannot be opened."<<endl;
		return 0;
	}
	init_file.close();

	ifstream init1_file("init_log.txt");

	if(init1_file.is_open())
	{
		while(getline(init1_file,initlog))
		{
			if((initlog[0]=='E') && (initlog[1] == 'r') && (initlog[2] == 'r') && (initlog[3] == 'o') && (initlog[4] == 'r'))
				return 0;
		}
	}
	else
	{
		IPD_log<<"Error : Response from ICount cannot be verified. File cannot be opened"<<endl;
		return 0;
	}
	return 1;
}

DWORD WINAPI readICount(LPVOID lpParameter)
{
	IPD_log<<"readICOunt"<<endl;
	char readbuffer[250];

	int bytesRead = 0;

	ofstream myfile("IPD_dataLog.txt");

	while(1)
	{
		bytesRead	= readFromSerialPort(lpParameter,readbuffer,99);
		readbuffer[bytesRead]=0;
		if(action == "start")
		{
			if (myfile.is_open())
			{
				if (readbuffer[0] != 0)
				myfile<<readbuffer<<endl;
			}
			else
			{
				IPD_log<<"Error : Data from ICount not logged. File cannot be opened."<<endl;
				exit (EXIT_FAILURE);
			}
		}
		if(action == "stop")
			break;
	}
	myfile.close();
	return 0;
}

DWORD WINAPI userEntry(LPVOID lpParameter)
{
	while(1)
	{
		cout<<"Enter 'start' : "<<endl;
		cin>>action;
		cout<<"You Entered : "<<action<<endl;
		if (action == "start")
			break;
		else
			cout<<"You have entered incorrect input.Please re-enter."<<endl;
	}

	while(1)
	{
		cout<<"Enter 'stop' : "<<endl;
		cin>>action;
		cout<<"You have entered : "<<action<<endl;

		if (action == "stop")
			break;
		else
			cout<<"You have entered incorrect input.Please re-enter."<<endl;
		}
	return 0;
}

int analyzeData()
{
	IPD_log<<"Analyze"<<endl;

	string buffer;

	int four_um = 0;
	int six_um = 0;
	int fourteen_um = 0;
	int thirty_um = 0;
	int four_um_avg = 0;
	int six_um_avg = 0;
	int fourteen_um_avg = 0;
	int thirty_um_avg = 0;
	int list_size = 0;

	map <int, list<int> > iso_map;
	map <int, list<int> > :: iterator iso_iter;
	map <int, list<int> > :: iterator iso_iter1;

	ifstream read_file("IPD_dataLog.txt");

	if(read_file.is_open())
	{
		while(getline(read_file,buffer))
		{
			if (buffer[0]=='I' && buffer[1]=='S' && buffer [2]=='O')
			{
				for( int a = 6; a<buffer.length(); a +=3)
				{
					int temp =0;
					char p[3];
					if (buffer [a] != '>')
					{
						p[0] = buffer[a];
						p[1] = buffer[a+1];
						p[2] = '\0';
						temp = atoi(p);
					}
					else
					{
						p[0] = buffer[a+1];
						p[1] = '\0';
						temp = atoi(p);
					}
					//Add to map
					iso_iter =  iso_map.find(a);
					if (iso_iter != iso_map.end())
					{
						iso_iter->second.push_back(temp);
					}
					else
					{
						list <int> iso_set;
						iso_set.push_back(temp);
						iso_map[a]=iso_set;
					}
				}	//End of For loop
			}		//End of if ISO
		}			//End of while
	}				//End of If file.is_open
	else
	{
		IPD_log<<"Cannot open file for analysis"<<endl;
		return 0;
	}
	read_file.close();

/*	//map print
	for (iso_iter1 = iso_map.begin(); iso_iter1 != iso_map.end(); iso_iter1++)

	{
		cout << iso_iter1->first << " : ";
		for (list <int> :: iterator list_iter1 = iso_iter1->second.begin(); list_iter1 != iso_iter1->second.end(); list_iter1++)
		{
			cout<<*list_iter1<<" ";
		}
		cout<<endl;
	}
*/
	list_size = iso_map[6].size();
	if (list_size <= 0)
	{
		IPD_log<<"No data logged."<<endl;
		IPD_log<<"End of Analyze."<<endl;
		return 1;
	}

	for (iso_iter1 = iso_map.begin(); iso_iter1 != iso_map.end(); iso_iter1++)
	{
		for (list <int> :: iterator list_iter1 = iso_iter1->second.begin(); list_iter1 != iso_iter1->second.end(); list_iter1++)
		{
			if((iso_iter1->first) == 6)
				four_um +=*list_iter1;
			if((iso_iter1->first) == 9)
				six_um += *list_iter1;
			if((iso_iter1->first) == 12)
				fourteen_um += *list_iter1;
			if((iso_iter1->first) == 15)
				thirty_um += *list_iter1;
		}
	}
	cout<<"List size : "<<list_size<<endl;

	four_um 	= four_um/list_size;
	six_um		= six_um /list_size;
	fourteen_um = fourteen_um/list_size;
	thirty_um 	= thirty_um  /list_size;

	cout<<"four _um_avg    : " <<four_um<<endl;
	cout<<"six_um_avg      : " <<six_um<<endl;
	cout<<"fourteen_um_avg : " <<fourteen_um<<endl;
	cout<<"thirty_um_avg   : " <<thirty_um<<endl;

	IPD_log<<"End of Analyze."<<endl; // to be removed
	return 1;
}
