#include <gflags/gflags.h>
#include <iostream>
#include <fstream>
#include <string>

DEFINE_string(filename, "", "pie file to test");


struct iIMDShape *iV_ProcessIMD(const std::string &filename, const char **ppFileData, const char *FileDataEnd);

void main(int argc, char** argv)
{
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	std::ifstream t(FLAGS_filename);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	const char* It = str.data();
	const char* E = It + str.size();
	iV_ProcessIMD(FLAGS_filename, &It, E);
}