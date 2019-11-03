#include "pch.h"
#include "DBconnector.h"
#include <mysql.h>
#include <iostream>
#include <sstream>
#include <cppconn/statement.h>
#include <vector>
using namespace std;

DBconnector::DBconnector()
{

}

DBconnector::~DBconnector()
{
}
void DBconnector::Connect(string ip, string username, string pass, string database)
{
	conn = mysql_init(0);

	conn = mysql_real_connect(conn, ip.c_str(), username.c_str(), pass.c_str(), database.c_str(), 3306, NULL, 0);
	if (conn)
	{
		cout << "Connected!" << endl;
	}
	else {
		throw "Failed To connect";
	}
}

int DBconnector::Register(std::string username)
{
	query = "INSERT INTO users(username,is_ai) VALUES('" + username + "',0)";
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}

	return Login(username);
}

int DBconnector::Login(string username)
{
	query = "SELECT userID FROM users WHERE username='" + username + "'";

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES *result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	if (row == NULL) {
		throw "No such user";
	}

	mysql_free_result(result);				//must be done after every SELECT query

	return atoi(row[0]);					//converts char* to int

	//cout << typeid(row[0]).name() << endl; � shows the datatype of a variable
}

void DBconnector::ChangeName(string newName, int userID)
{
	ss << userID;
	query = "UPDATE users SET username='" + newName + "' WHERE userID=" + ss.str();
	ss.clear();
	ss.str(string());
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
}

pair<int, int> DBconnector::GetInfo(int userID)
{
	ss << userID;
	int wins, losses;
	query = "SELECT wins,losses FROM users WHERE userID=" + ss.str();
	ss.clear();
	ss.str(string());
	MYSQL_RES *result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	pair<int, int> stats = make_pair(atoi(row[0]), atoi(row[1]));

	mysql_free_result(result);

	return stats;
}

int DBconnector::GetLobbyID(int userID) {		//if used in this .cpp must be in a throw throw
	ss << userID;

	int lobbyID;
	query = "SELECT current_lobbyID FROM users WHERE userID=" + ss.str();

	ss.clear();
	ss.str(string());

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES *result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	if (row[0] == NULL) {
		lobbyID = -1;
	}
	else {
		lobbyID = atoi(row[0]);
	}

	mysql_free_result(result);

	return lobbyID;
}

void DBconnector::SetUserLobby(int userID, int lobbyID) {
	ss << userID;
	string uID = ss.str();
	ss.clear();
	ss.str(string());
	ss << lobbyID;
	query = "UPDATE users SET current_lobbyID=" + ss.str() + " WHERE userID=" + uID;
	ss.clear();
	ss.str(string());
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
}

int DBconnector::CreateLobby(string lobbyName, int userID)	//1.Check if player is already in a lobby 2.Create a lobby 3.Update current_lobbyID in user table 4. return lobbyID
{
	int cLobby;
	try
	{
		cLobby = GetLobbyID(userID);
	}
	catch (const char* e)
	{
		throw e;
	}

	if (cLobby != -1) {
		return cLobby;		//if a player is in a lobby return the lobbyID
	}

	ss << userID;
	query = "INSERT INTO lobbies(lobby_name,adminID) VALUES('" + lobbyName + "'," + ss.str() + ")";
	ss.clear();
	ss.str(string());
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}

	query = "SELECT LAST_INSERT_ID()";	//get last inserted row index

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}

	MYSQL_RES *result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	cLobby = atoi(row[0]);
	mysql_free_result(result);
	try
	{
		SetUserLobby(userID, cLobby);
	}
	catch (const char* e)
	{
		throw e;
	}

	return atoi(row[0]);
}

void  DBconnector::LeaveLobby(int lobbyID, int userID)
{
	ss << lobbyID;
	query = "SELECT lobby_status FROM lobbies WHERE lobbyID=" + ss.str();

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES *result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	cout << "before: " << row[0] << endl;
	if (row[0][0] != 'w') {
		throw "Already in game";
	}
	mysql_free_result(result);

	query = "DELETE FROM lobbies WHERE lobbyID=" + ss.str();
	ss.clear();
	ss.str(string());
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}

	try
	{
		SetUserLobby(userID, -1);
	}
	catch (const char* e)
	{
		throw e;
	}

}

vector<DBconnector::LobbyTable> DBconnector::ListLobbies(boolean isUser)
{
	vector<LobbyTable> lobbyTable;	
	if (isUser) {
		query="SELECT l.lobbyID, l.lobby_name, u1.username FROM lobbies AS l JOIN users AS u1 ON u1.userID=l.adminID WHERE l.lobby_status='w'";
	}
	else 
	{
		query = "SELECT l.lobbyID, l.lobby_name, u1.username, u2.username FROM lobbies AS l JOIN users AS u1 ON u1.userID=l.adminID LEFT JOIN users AS u2 ON u2.userID=l.opponentID WHERE l.lobby_status!='w'";
	}
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES *result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row;
	int rowCount = result->row_count;	//get number of lobbies
	
	for (int i = 0; i < rowCount; i++) {
		row = mysql_fetch_row(result);
		LobbyTable tempLobby;
		tempLobby.lobbyID = atoi(row[0]);
		tempLobby.lobbyName = row[1];
		tempLobby.adminName = row[2];
		lobbyTable.push_back(tempLobby);
	}

	mysql_free_result(result);

	return lobbyTable;
}


void DBconnector::JoinLobbyAsPlayer(int lobbyID, int userID)
{
	ss >> userID;
	string uID = ss.str();
	ss.clear();
	ss.str(string());
	ss >> lobbyID;
	query = "UPDATE lobbies SET opponentID='"+uID+"', lobby_status='r' WHERE lobbyID='" + ss.str();
	ss.clear();
	ss.str(string());
	
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	try {
		SetUserLobby(userID, lobbyID);
	}
	catch(const char* e){
		cout << e << endl;
	}
}

DBconnector::Rlobby DBconnector::ReadLobby(int lobbyID)
{
	ss >> lobbyID;
	query = "SELECT game_status, console_output, current_player FROM lobbies WHERE lobbyID="+lobbyID;	
	ss.clear();
	ss.str(string());
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES* result = mysql_store_result(conn);
	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	Rlobby rlobby;
	rlobby.game_status = row[0];
	rlobby.console_output = row[1];
	rlobby.curr_player = atoi(row[2]);
	return rlobby;
}

pair<string, string> DBconnector::ReadMap(int lobbyID)
{
	ss << lobbyID;
	string map1, map2;
	query = "SELECT admin_map, opponent_map FROM lobbies WHERE lobbyID=" + ss.str();
	ss.clear();
	ss.str(string());
	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES* result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	map1 = row[0];
	map2 = row[1];

	mysql_free_result(result);

	return make_pair(map1, map2);
}

void DBconnector::WriteMove(int lobbyID, string userIn)
{
	ss << lobbyID;
	query = "UPDATE lobbies SET user_input='" + userIn + "', game_status='c' WHERE lobbyID=" + ss.str();
	ss.clear();
	ss.str(string());

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
}

int DBconnector::AcknowledgeLoss(int lobbyID, int userID)
{
	int historyID;
	ss << lobbyID;
	query = "SELECT historyID FROM lobbies WHERE lobbyID=" + ss.str(); //get pointer to game history

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES* result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	historyID = atoi(row[0]);

	mysql_free_result(result);

	query = "UPDATE lobbies SET user_input='OK', game_status='c' WHERE lobbyID=" + ss.str();

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}

	try
	{
		SetUserLobby(userID, -1);
	}
	catch (const char* e)
	{
		throw e;
	}
	ss.clear();
	ss.str(string());

	return historyID;
}

string DBconnector::GetWinner(int gameID)
{
	string username;
	ss >> gameID;
	query = "SELECT username FROM users WHERE userID= SELECT winnerID FROM history WHERE gameID ="+ss.str();
	ss.clear();
	ss.str(string());

	if (PassQuery(query) != 0) {
		throw mysql_error(conn);
	}
	MYSQL_RES* result = mysql_store_result(conn);

	if (result == NULL) {
		throw mysql_error(conn);
	}

	MYSQL_ROW row = mysql_fetch_row(result);

	username = row[0];

	return username;
}



MYSQL_RES DBconnector::GetInfoOnGame(int)
{
	query = "";
	return MYSQL_RES();
}







