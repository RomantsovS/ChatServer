#include <iostream>
#include <uwebsockets/App.h>
#include <string>
#include <regex>
#include <map>
#include <algorithm>
#include <sstream>

using std::string;
using std::cout;
using std::endl;
using std::regex;
using std::regex_match;
using std::map;
using std::cin;

struct UserConnection {
	string name;
	unsigned long user_id;
};

string botMsg(const map<string, string>& database, const std::string& text);

int main()
{
	int port = 8888;
	unsigned long latest_user_id = 10;

	const string private_channel_name = "user#";

	map<unsigned long, string> online_users;

	map<string, string> database = {
	   {"hello", "Oh, hello to you hooman"},
	   {"how are you", "I'm good"},
	   {"what are you doing", "I'm answering stupid question"},
	   {"what is your name", "My name is Skill Bot 3000"},
	   {"exit", "Ok byeeeee!"},
	};

	uWS::App().ws<UserConnection>("/*", {
		.open = [&latest_user_id, &private_channel_name, &online_users](auto* ws) {
		UserConnection* data = static_cast<UserConnection*>(ws->getUserData());
		data->user_id = latest_user_id++;
		data->name = "UNNAMED";

		cout << "New user connected ID = " << data->user_id << endl;
		ws->subscribe("broadcast");
		ws->subscribe(private_channel_name + std::to_string(data->user_id));

		online_users[data->user_id] = data->name;

		cout << "Total users connected: " << online_users.size() << endl; // ex 1
	},
		.message = [&private_channel_name, &online_users, &database](auto* ws, std::string_view message, uWS::OpCode opCode) {
		string SET_NAME("SET_NAME=");
		string MESSAGE_TO("MESSAGE_TO=");

		UserConnection* data = static_cast<UserConnection*>(ws->getUserData());

		cout << "New message received " << message << endl;

		if (message.find("SET_NAME=") == 0) {
			cout << "User sets their name\n";

			auto name = message.substr(SET_NAME.size());

			if (name.find(",") != std::string_view::npos) { // ex 2
				ws->publish(private_channel_name + std::to_string(data->user_id),
					"Error. Name contains \",\"");
			}
			else if (name.size() > 255) { // ex 3
				ws->publish(private_channel_name + std::to_string(data->user_id),
					"Error. Name too long");
			}
			else {
				data->name = message.substr(SET_NAME.size());

				online_users[data->user_id] = data->name;
				ws->publish("broadcast", "NEW_USER=" + std::to_string(data->user_id) + "," + data->name);
			}
		}
		if (message.find("MESSAGE_TO=") == 0) {
			cout << "User sends private message\n";
			auto rest = message.substr(MESSAGE_TO.size());
			int comma_position = rest.find(",");
			auto ID = static_cast<string>(rest.substr(0, comma_position));
			auto text = rest.substr(comma_position + 1);

			if (ID == "1") { // ex 5
				auto bot_answer = botMsg(database, static_cast<string>(text));
				ws->publish(private_channel_name + std::to_string(data->user_id), bot_answer);
			}
			else if (online_users.find(std::stoi(ID)) == online_users.end()) { // ex 4
				ws->publish(private_channel_name + std::to_string(data->user_id),
					"Error, there is no user with ID = " + ID);
			}
			else {
				ws->publish(private_channel_name + ID, text);
			}
		}
	},
		.close = [&online_users](auto* ws, int code, std::string_view message) {
		UserConnection* data = static_cast<UserConnection*>(ws->getUserData());
		online_users.erase(data->user_id);

		ws->publish("broadcast", "User " + data->name + " with ID = " + std::to_string(data->user_id) + " disconnected\n");
	}
		}).listen(port, [port](auto* token) {
		if (token) {
			cout << "Server started successfully on port " << port << endl;
		}
		else {
			cout << "Server failed to start" << endl;
		}
	}).run();
}

void to_lower(string& str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

string bot(string text) {
	return "[BOT]: " + text;
}

string botMsg(const map<string, string>& database, const std::string& text) {
	string question;
	std::istringstream is(text);

	string answer;
	
	while (true) {
		if (!std::getline(is, question))
			return answer;

		to_lower(question);

		bool isAnswerFound = false;
		
		for (auto entry : database) {
			regex pattern = regex(".*" + entry.first + ".*");
			if (regex_match(question, pattern)) {
				if (!answer.empty()) {
					answer += "\n";
				}
				answer += bot(entry.second);
				isAnswerFound = true;
			}
		}
		
		if (!isAnswerFound) {
			if (!answer.empty()) {
				answer += "\n";
			}
			answer += bot("Sorry I do not understand");
		}
	}

	return answer;
}