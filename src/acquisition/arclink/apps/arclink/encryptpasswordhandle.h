#ifndef ENCRYPTPASSWORDHANDLE_H
#define ENCRYPTPASSWORDHANDLE_H

#include <iostream>
#include <exception>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <sys/time.h>
#include <openssl/evp.h>

#include "encrypterror.h"

namespace SSLWrapper {

class EncryptPasswordHandle {
	
private:
	std::string filename;
	std::string masterpassword;
	std::string letters;
	std::string dcid;
	std::string dcname;
	std::string dcemail;
	
public:
	EncryptPasswordHandle();
	EncryptPasswordHandle(std::string filename, std::string masterpassword, std::string dcid, std::string dcname, std::string dcemail);

private:
	// Check if the decrypted password contain only valid letters.
	bool isGood(std::string password);
	// Hex2Binary
	unsigned char* h2b(std::string s, int *len);
	// Binary2Hex
	std::string b2h(const unsigned char *buf, int len);
	// Make a cypher context
	EVP_CIPHER_CTX* makeContext(int direction);
	// Encrypt a string using password
	std::string encrypt(std::string input);
	// Decrypt a string using password
	std::string decrypt(std::string input);

	// Method that send email, use mailx tool !
	int Email(std::string username, std::string password);
	
	// Generate a new password based on the current time (microseconds)
	// The final password will can have letters, numbers and some ponctuations.
	// 1 & l are excluded to avoid confusion.
	std::string newPassword(int max);
	
	std::string tReplace(std::string input, std::string pattern, std::string value);
	
public:
	// Open the password file and find it
	std::string findPassword(std::string username, bool cancreate = false, bool emailpassword = false);
};

}
#endif // ENCRYPTPASSWORDHANDLE_H
