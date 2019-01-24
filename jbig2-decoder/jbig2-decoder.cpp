// jbig2-decoder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <string>
#include <vector>
#include <fstream>
#include <istream>
#include <ostream>
#include <iostream>
#include <iterator>
#include <chrono>
#include "support/module.h"
#include "support/getopt_pp.h"
#include "jbig2/JBig2_Context.h"
#include "jbig2/JBig2_Image.h"

std::vector<unsigned char> readFile(const std::string& filename);
void WritePbm(CJBig2_Image* jbig2_img, const char* fname);

double PCFreq = 0.0;
__int64 CounterStart = 0;

int main(int argc, char *argv[])
{
	GetOpt::GetOpt_pp ops(argc, argv);

	ops.exceptions(std::ios::failbit | std::ios::eofbit);
	std::string output_file;
	std::string output_format;
	std::string input_file;
	std::string input_global_stream_file;
	bool should_output_processing_time = false;
	bool has_global_stream = false;
	std::vector<std::string> args;

	try {
		ops >> GetOpt::Option('o', "output-file", output_file, "");
		ops >> GetOpt::Option('f', "format", output_format, "");
		ops >> GetOpt::OptionPresent('t', "time", should_output_processing_time);
		ops >> GetOpt::GlobalOption(args);
	} catch (GetOpt::GetOptEx ex) {
		std::cerr << "Error in arguments" << std::endl;
		return -1;
	}

	if (args.size() > 2) {
		if (args.size() > 0)
			std::cerr << "too many input files provided" << std::endl;
		return -1;
	}

	if (args.size() == 1) {
		input_file = args[0];
	} else {
		input_global_stream_file = args[0];
		input_file = args[1];
		has_global_stream = true;
	}

	JBig2Module module;

	auto data = readFile(input_file);
	FX_DWORD sz = static_cast<FX_DWORD>(data.size());

	CJBig2_Context* cntxt = nullptr;
	if (! has_global_stream) {
		cntxt = CJBig2_Context::CreateContext(&module, nullptr, 0,
			reinterpret_cast<FX_BYTE*>(&(data[0])), sz, JBIG2_FILE_STREAM);
	} else {
		auto global_data = readFile(input_global_stream_file);
		cntxt = CJBig2_Context::CreateContext(
			&module, 
			reinterpret_cast<FX_BYTE*>(&(global_data[0])), static_cast<FX_DWORD>(global_data.size()),
			reinterpret_cast<FX_BYTE*>(&(data[0])), sz, 
			JBIG2_EMBED_STREAM 
		);
	}
	if (!cntxt) {
		std::cerr << "Error creating jbig2 context" << std::endl;
		return -1;
	 }

	std::chrono::duration<double> elapsed_time;
	std::vector<CJBig2_Image*> pages;
	FX_INT32 result = JBIG2_SUCCESS;
	std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
	while (cntxt->GetProcessiveStatus() == FXCODEC_STATUS_FRAME_READY && result == JBIG2_SUCCESS) {
		if (cntxt->GetProcessiveStatus() == FXCODEC_STATUS_ERROR)
			throw std::runtime_error("cntxt->GetProcessiveStatus() == FXCODEC_STATUS_ERROR");
		CJBig2_Image* img = nullptr;
		result = cntxt->getNextPage(&img, nullptr);
		if (img)
			pages.push_back(img);
	}
	elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
	if (should_output_processing_time) 
		std::cout << "decoding input took " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() << " ms\n";

	WritePbm(pages[0], "foo.pbm");
}

void WritePbm(CJBig2_Image* jbig2_img, const char* fname)
{
	std::fstream file;
	file = std::fstream(fname, std::ios::out | std::ios::binary);
	file << "P4\n" << jbig2_img->m_nWidth << " " << jbig2_img->m_nHeight << "\n";
	//file.write((const char*)jbig2_img->m_pData, jbig2_img->m_nStride * jbig2_img->m_nHeight);
	const char* data = (const char*)jbig2_img->m_pData;
	int stride = jbig2_img->m_nStride - 1;
	for(int i = 0; i < jbig2_img->m_nHeight; i++, data += stride+1)
		file.write(data, stride);
}

std::vector<unsigned char> readFile(const std::string& filename)
{
	// open the file:
	std::ifstream file(filename, std::ios::binary);

	// Stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	// get its size:
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	
	// reserve capacity
	std::vector<unsigned char> vec;
	vec.reserve(fileSize);
	
	// read the data:
	vec.insert(vec.begin(),
		std::istream_iterator<unsigned char>(file),
		std::istream_iterator<unsigned char>()
	);
	
	return vec;
}
