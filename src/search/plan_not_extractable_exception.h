#include <exception>

#include <string>
using std::string;
using std::exception;

class PlanNotExtractableException: public exception {
private:
	string message;

public:
	PlanNotExtractableException(string msg) :
			message(msg) {
	}
	virtual const char* what() const throw () {
		return message.c_str();
	}
};
