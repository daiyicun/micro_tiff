#pragma once
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <direct.h>
#include <queue>
#include "..\..\src\turbojpeg-2.0.6\turbojpeg.h"
#include "..\..\src\ImageStoreLibrary\classic_tiff_library.h"
#include "HighPerformanceTimer.h"
#include <functional>

using namespace std;
using namespace tiff;

#define SRC_PATH (SOLUTION_DIR"..\\..\\bin\\test_data\\sample.jpg")
#define SRC_PATH2 (SOLUTION_DIR"..\\..\\bin\\test_data\\sample_1.jpg")

void Load_File(const wchar_t* name_ext, tiff::CompressionMode compress_mode)
{
	struct _stat64 statbuf;
	int r = _stat64(SRC_PATH, &statbuf);
	ASSERT_FALSE(r || !(statbuf.st_mode & S_IFREG));

	auto fileSize = statbuf.st_size;
	std::unique_ptr<void, std::function<void(void*)>> auto_buffer(malloc(fileSize + !fileSize), free);
	void* const buffer = auto_buffer.get();

	FILE* f;
	fopen_s(&f, SRC_PATH, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, f);
	fclose(f);
	ASSERT_EQ(readSize, fileSize);

	//Decompress Jpeg file to RGB
	int ret = -1;
	tjhandle handle = tjInitDecompress();
	ASSERT_FALSE(handle == nullptr);

	int width, height, subsample, colorspace;
	ret = tjDecompressHeader3(handle, (unsigned char*)buffer, (unsigned long)fileSize, &width, &height, &subsample, &colorspace);
	ASSERT_EQ(ret, 0);

	auto size = tjBufSize(width, height, subsample);
	unsigned char* buf = (unsigned char*)malloc(size);
	memset(buf, 0, size);
	ret = tjDecompress2(handle, (unsigned char*)buffer, (unsigned long)fileSize, buf, width, 0, height, TJPF_RGB, TJFLAG_ACCURATEDCT);
	ASSERT_EQ(ret, 0);

	tjDestroy(handle);

	wchar_t *s_cwd = _wgetcwd(NULL, 0);
	ASSERT_FALSE(s_cwd == NULL);

	wchar_t* w_single_path = new wchar_t[256];
	swprintf_s(w_single_path, 256, L"%s\\test\\%s.tif", s_cwd, name_ext);

	int save_hdl = open_tiff(w_single_path, tiff::OpenMode::CREATE_MODE);
	ASSERT_GE(save_hdl, 0);

	tiff::SingleImageInfo save_info =
	{
		(unsigned int)width,
		(unsigned int)height,
		8,
		tiff::PixelType::PIXEL_UINT8,
		tiff::ImageType::IMAGE_RGB,
		compress_mode
	};

	long frame_number1 = create_image(save_hdl, save_info);
	ASSERT_GE(frame_number1, 0);

	long frame_number2 = create_image(save_hdl, save_info);
	ASSERT_GE(frame_number2, 0);

	long status = save_image_data(save_hdl, frame_number1, buf);
	ASSERT_EQ(status, 0);

	status = save_image_data(save_hdl, frame_number2, buf);
	ASSERT_EQ(status, 0);

	free(buf);

	wchar_t* model = L"Channel中文";
	size_t model_size = wcslen(model) * sizeof(wchar_t);
	status = set_image_tag(save_hdl, frame_number1, 272, tiff::TiffTagDataType::TIFF_BYTE, (unsigned short)model_size,model);
	ASSERT_EQ(status, 0);

	uint32_t color = 0xFF00FF00;
	status = set_image_tag(save_hdl, frame_number1, 10599, tiff::TiffTagDataType::TIFF_LONG, 1, &color);
	ASSERT_EQ(status, 0);

	close_tiff(save_hdl);

	int load_hdl = open_tiff(w_single_path, tiff::OpenMode::READ_ONLY_MODE);
	ASSERT_GE(load_hdl, 0);

	long frame_number3 = create_image(load_hdl, save_info);
	ASSERT_LT(frame_number3, 0);

	unsigned int count;
	status = get_image_count(load_hdl, &count);
	ASSERT_EQ(status, 0);

	void* tag_model = malloc(model_size);
	status = get_image_tag(load_hdl, 0, 272, (unsigned short)model_size, tag_model);
	ASSERT_EQ(status, 0);
	free(tag_model);

	tiff::SingleImageInfo load_info;
	status = get_image_info(load_hdl, 0, &load_info);
	ASSERT_EQ(status, 0);
	ASSERT_EQ(load_info.width, save_info.width);
	ASSERT_EQ(load_info.height, save_info.height);
	ASSERT_EQ(load_info.valid_bits, save_info.valid_bits);
	ASSERT_EQ(load_info.pixel_type, save_info.pixel_type);
	ASSERT_EQ(load_info.image_type, save_info.image_type);
	ASSERT_EQ(load_info.compress_mode, save_info.compress_mode);

	void* load_buf = malloc(load_info.width * load_info.height * 3u);
	load_image_data(load_hdl, 0, load_buf);

	close_tiff(load_hdl);
	free(load_buf);
	delete w_single_path;
}

void Change_Tag(const wchar_t* name_ext, tiff::CompressionMode compress_mode)
{
	struct _stat64 statbuf;
	int r = _stat64(SRC_PATH, &statbuf);
	ASSERT_FALSE(r || !(statbuf.st_mode & S_IFREG));

	auto fileSize = statbuf.st_size;
	std::unique_ptr<void, std::function<void(void*)>> auto_buffer(malloc(fileSize + !fileSize), free);
	void* const buffer = auto_buffer.get();

	FILE* f;
	fopen_s(&f, SRC_PATH, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, f);
	fclose(f);
	ASSERT_EQ(readSize, fileSize);

	//Decompress Jpeg file to RGB
	int ret = -1;
	tjhandle handle = tjInitDecompress();
	ASSERT_FALSE(handle == nullptr);

	int width, height, subsample, colorspace;
	ret = tjDecompressHeader3(handle, (unsigned char*)buffer, (unsigned long)fileSize, &width, &height, &subsample, &colorspace);
	ASSERT_EQ(ret, 0);

	auto size = tjBufSize(width, height, subsample);
	unsigned char* buf = (unsigned char*)malloc(size);
	memset(buf, 0, size);
	ret = tjDecompress2(handle, (unsigned char*)buffer, (unsigned long)fileSize, buf, width, 0, height, TJPF_RGB, TJFLAG_ACCURATEDCT);
	ASSERT_EQ(ret, 0);

	tjDestroy(handle);

	wchar_t* s_cwd = _wgetcwd(NULL, 0);
	ASSERT_FALSE(s_cwd == NULL);

	wchar_t* w_single_path = new wchar_t[256];
	swprintf_s(w_single_path, 256, L"%s\\test\\%s.tif", s_cwd, name_ext);

	int save_hdl = open_tiff(w_single_path, tiff::OpenMode::CREATE_MODE);
	ASSERT_GE(save_hdl, 0);

	tiff::SingleImageInfo save_info =
	{
		(unsigned int)width,
		(unsigned int)height,
		8,
		tiff::PixelType::PIXEL_UINT8,
		tiff::ImageType::IMAGE_RGB,
		compress_mode
	};

	long frame_number1 = create_image(save_hdl, save_info);
	ASSERT_GE(frame_number1, 0);

	/*long frame_number2 = create_image(save_hdl, save_info);
	ASSERT_GE(frame_number2, 0);*/

	long status = save_image_data(save_hdl, frame_number1, buf);
	ASSERT_EQ(status, 0);

	//status = save_image_data(save_hdl, frame_number2, buf);
	//ASSERT_EQ(status, 0);

	free(buf);

	wchar_t* model = L"Channel中文";
	size_t model_size = wcslen(model) * sizeof(wchar_t);
	void* buf_tag = calloc(64, 1);
	memcpy(buf_tag, model, model_size);
	status = set_image_tag(save_hdl, frame_number1, 272, tiff::TiffTagDataType::TIFF_BYTE, 64, buf_tag);
	free(buf_tag);
	ASSERT_EQ(status, 0);

	uint32_t color = 0xFF00FF00;
	status = set_image_tag(save_hdl, frame_number1, 10599, tiff::TiffTagDataType::TIFF_LONG, 1, &color);
	ASSERT_EQ(status, 0);

	close_tiff(save_hdl);

	int load_hdl = open_tiff(w_single_path, tiff::OpenMode::READ_WRITE_MODE);
	ASSERT_GE(load_hdl, 0);

	/*long frame_number3 = create_image(load_hdl, save_info);
	ASSERT_LT(frame_number3, 0);*/

	unsigned int count;
	status = get_image_count(load_hdl, &count);
	ASSERT_EQ(status, 0);

	wchar_t* anothrer_model = L"Channel中文急啊急啊急";
	size_t anothrer_model_size = wcslen(anothrer_model) * sizeof(wchar_t);
	void* buf_anothrer = calloc(64, 1);
	memcpy(buf_anothrer, anothrer_model, anothrer_model_size);
	status = set_image_tag(save_hdl, 0, 272, tiff::TiffTagDataType::TIFF_BYTE, 64, buf_anothrer);
	free(buf_anothrer);
	ASSERT_EQ(status, 0);

	uint32_t clor = 0xFFAAFFBB;
	status = set_image_tag(save_hdl, 0, 10599, tiff::TiffTagDataType::TIFF_LONG, 1, &clor);
	ASSERT_EQ(status, 0);

	void* tag_model_anothrer = calloc(64, 1);
	status = get_image_tag(load_hdl, 0, 272, 64, tag_model_anothrer);
	ASSERT_EQ(status, 0);
	free(tag_model_anothrer);

	tiff::SingleImageInfo load_info;
	status = get_image_info(load_hdl, 0, &load_info);
	ASSERT_EQ(status, 0);
	ASSERT_EQ(load_info.width, save_info.width);
	ASSERT_EQ(load_info.height, save_info.height);
	ASSERT_EQ(load_info.valid_bits, save_info.valid_bits);
	ASSERT_EQ(load_info.pixel_type, save_info.pixel_type);
	ASSERT_EQ(load_info.image_type, save_info.image_type);
	ASSERT_EQ(load_info.compress_mode, save_info.compress_mode);

	void* load_buf = malloc(load_info.width * load_info.height * 3u);
	load_image_data(load_hdl, 0, load_buf);

	close_tiff(load_hdl);
	free(load_buf);
	delete w_single_path;
}

void Append_File(const wchar_t* name_ext, tiff::CompressionMode compress_mode)
{
	Load_File(name_ext, tiff::CompressionMode::COMPRESSIONMODE_LZW);

	struct _stat64 statbuf;
	int r = _stat64(SRC_PATH2, &statbuf);
	ASSERT_FALSE(r || !(statbuf.st_mode & S_IFREG));

	auto fileSize = statbuf.st_size;
	std::unique_ptr<void, std::function<void(void*)>> auto_buffer(malloc(fileSize + !fileSize), free);
	void* const buffer = auto_buffer.get();

	FILE* f;
	fopen_s(&f, SRC_PATH2, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, f);
	fclose(f);
	ASSERT_EQ(readSize, fileSize);

	//Decompress Jpeg file to RGB
	int ret = -1;
	tjhandle handle = tjInitDecompress();
	ASSERT_FALSE(handle == nullptr);

	int width, height, subsample, colorspace;
	ret = tjDecompressHeader3(handle, (unsigned char*)buffer, (unsigned long)fileSize, &width, &height, &subsample, &colorspace);
	ASSERT_EQ(ret, 0);

	auto size = tjBufSize(width, height, subsample);
	unsigned char* buf = (unsigned char*)malloc(size);
	memset(buf, 0, size);
	ret = tjDecompress2(handle, (unsigned char*)buffer, (unsigned long)fileSize, buf, width, 0, height, TJPF_RGB, TJFLAG_ACCURATEDCT);
	ASSERT_EQ(ret, 0);

	tjDestroy(handle);

	wchar_t* s_cwd = _wgetcwd(NULL, 0);
	ASSERT_FALSE(s_cwd == NULL);

	wchar_t single_path[256];
	swprintf_s(single_path, 256, L"%s\\test\\%s.tif", s_cwd, name_ext);

	int save_hdl = open_tiff(single_path, tiff::OpenMode::READ_WRITE_MODE);
	ASSERT_GE(save_hdl, 0);

	tiff::SingleImageInfo save_info =
	{
		(unsigned int)width,
		(unsigned int)height,
		8,
		tiff::PixelType::PIXEL_UINT8,
		tiff::ImageType::IMAGE_RGB,
		compress_mode
	};

	long frame_number = create_image(save_hdl, save_info);
	ASSERT_GE(frame_number, 0);

	long status = save_image_data(save_hdl, frame_number, buf);
	ASSERT_EQ(status, 0);

	close_tiff(save_hdl);

	free(buf);
}

struct saved_file
{
	const wchar_t* file_name;
	int width;
	int height;
};
queue<saved_file> saved_files;
mutex mut;

static size_t m_total_count;
static size_t m_current_count;

void thread_for_save(const wchar_t* name_ext, int count,tiff::CompressionMode compress_mode)
{
	struct _stat64 statbuf;
	int r = _stat64(SRC_PATH, &statbuf);
	ASSERT_FALSE(r || !(statbuf.st_mode & S_IFREG));

	auto fileSize = statbuf.st_size;
	std::unique_ptr<void, std::function<void(void*)>> auto_buffer(malloc(fileSize + !fileSize), free);
	void* const buffer = auto_buffer.get();

	FILE* f;
	fopen_s(&f, SRC_PATH, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, f);
	fclose(f);
	ASSERT_EQ(readSize, fileSize);

	//Decompress Jpeg file to RGB
	int ret = -1;
	tjhandle handle = tjInitDecompress();
	ASSERT_FALSE(handle == nullptr);

	int width, height, subsample, colorspace;
	ret = tjDecompressHeader3(handle, (unsigned char*)buffer, (unsigned long)fileSize, &width, &height, &subsample, &colorspace);
	ASSERT_EQ(ret, 0);

	auto size = tjBufSize(width, height, subsample);
	unsigned char* buf = (unsigned char*)malloc(size);
	memset(buf, 0, size);
	ret = tjDecompress2(handle, (unsigned char*)buffer, (unsigned long)fileSize, buf, width, 0, height, TJPF_RGB, TJFLAG_ACCURATEDCT);
	ASSERT_EQ(ret, 0);

	tjDestroy(handle);

	wchar_t*s_cwd = _wgetcwd(NULL, 0);
	ASSERT_FALSE(s_cwd == NULL);

	for (int i = 0; i < count; i++)
	{
		wchar_t* single_path = new wchar_t[256];
		swprintf_s(single_path, 256, L"%s\\test\\%s_MT_%d.tif", s_cwd, name_ext, i);
		int hdl = open_tiff(single_path, tiff::OpenMode::CREATE_MODE);
		ASSERT_GE(hdl, 0);
		tiff::SingleImageInfo info =
		{
			(unsigned int)width,
			(unsigned int)height,
			8,
			tiff::PixelType::PIXEL_UINT8,
			tiff::ImageType::IMAGE_RGB,
			compress_mode
		};
		long frame_number = create_image(hdl, info);
		ASSERT_EQ(frame_number, 0);

		long status = save_image_data(hdl, frame_number, buf);
		ASSERT_EQ(status, 0);

		close_tiff(hdl);
		unique_lock<mutex> lck(mut);
		saved_files.push({ single_path, width, height });
	}
	free(buf);
}

void load_single_frame(saved_file file, tiff::CompressionMode compress_mode)
{
	int hdl = open_tiff(file.file_name, tiff::OpenMode::READ_ONLY_MODE);
	ASSERT_GE(hdl, 0);
	tiff::SingleImageInfo info;
	long status = get_image_info(hdl, 0, &info);
	ASSERT_EQ(status, 0);
	ASSERT_EQ(info.width, (unsigned int)file.width);
	ASSERT_EQ(info.height, (unsigned int)file.height);
	ASSERT_EQ(info.pixel_type, tiff::PixelType::PIXEL_UINT8);
	ASSERT_EQ(info.image_type, tiff::ImageType::IMAGE_RGB);
	ASSERT_EQ(info.valid_bits, 8);
	ASSERT_EQ(info.compress_mode, compress_mode);

	void* read_buffer = malloc(info.width * info.height * 3);
	status = load_image_data(hdl, 0, read_buffer);
	free(read_buffer);
	close_tiff(hdl);
	delete(file.file_name);
	ASSERT_EQ(status, 0);	
}

void thread_for_load(tiff::CompressionMode compress_mode)
{
	while (m_current_count < m_total_count)
	{
		Sleep(1000);
		unique_lock<mutex> lck(mut);
		vector<thread> threads;
		m_current_count += saved_files.size();
		while (!saved_files.empty())
		{
			saved_file file = saved_files.front();
			saved_files.pop();
			threads.push_back(thread(load_single_frame, file, compress_mode));
		}		
		for (auto& t : threads)
		{
			t.join();
		}
		threads.clear();
		lck.unlock();
	}
}

void Save_Load_MultiThread(const wchar_t* name_ext, int count, tiff::CompressionMode compress_mode)
{
	m_total_count = count;
	m_current_count = 0;
	
	thread th_save(thread_for_save, name_ext, count, compress_mode);
	thread th_load(thread_for_load, compress_mode);
	th_load.join();
	th_save.join();
}


void compare_performance()
{
	struct _stat64 statbuf;
	int r = _stat64(SRC_PATH, &statbuf);
	ASSERT_FALSE(r || !(statbuf.st_mode & S_IFREG));

	auto fileSize = statbuf.st_size;
	std::unique_ptr<void, std::function<void(void*)>> auto_buffer(malloc(fileSize + !fileSize), free);
	void* const buffer = auto_buffer.get();

	FILE* f;
	fopen_s(&f, SRC_PATH, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, f);
	fclose(f);
	ASSERT_EQ(readSize, fileSize);

	//Decompress Jpeg file to RGB
	int ret = -1;
	tjhandle handle = tjInitDecompress();
	ASSERT_FALSE(handle == nullptr);

	int width, height, subsample, colorspace;
	ret = tjDecompressHeader3(handle, (unsigned char*)buffer, (unsigned long)fileSize, &width, &height, &subsample, &colorspace);
	ASSERT_EQ(ret, 0);

	auto size = tjBufSize(width, height, subsample);
	unsigned char* buf = (unsigned char*)malloc(size);
	memset(buf, 0, size);
	ret = tjDecompress2(handle, (unsigned char*)buffer, (unsigned long)fileSize, buf, width, 0, height, TJPF_RGB, TJFLAG_ACCURATEDCT);
	ASSERT_EQ(ret, 0);

	tjDestroy(handle);

	wchar_t* s_cwd = _wgetcwd(NULL, 0);
	ASSERT_FALSE(s_cwd == NULL);

	wchar_t single_path1[256];
	swprintf_s(single_path1, 256, L"%s\\test\\%s.tif", s_cwd, L"CMP_LZW");
	SingleImageInfo info1 =
	{
		(uint32_t)width,
		(uint32_t)height,
		8,
		tiff::PixelType::PIXEL_UINT8,
		tiff::ImageType::IMAGE_RGB,
		tiff::CompressionMode::COMPRESSIONMODE_LZW
	};

	wchar_t single_path2[256];
	swprintf_s(single_path2, 256, L"%s\\test\\%s.tif", s_cwd, L"CMP_ZIP");
	SingleImageInfo info2 =
	{
		(uint32_t)width,
		(uint32_t)height,
		8,
		tiff::PixelType::PIXEL_UINT8,
		tiff::ImageType::IMAGE_RGB,
		tiff::CompressionMode::COMPRESSIONMODE_ZIP
	};

	wchar_t single_path3[256];
	swprintf_s(single_path3, 256, L"%s\\test\\%s.tif", s_cwd, L"CMP_Raw");
	SingleImageInfo info3 =
	{
		(uint32_t)width,
		(uint32_t)height,
		8,
		tiff::PixelType::PIXEL_UINT8,
		tiff::ImageType::IMAGE_RGB,
		tiff::CompressionMode::COMPRESSIONMODE_NONE
	};

	wchar_t single_path4[256];
	swprintf_s(single_path4, 256, L"%s\\test\\%s.tif", s_cwd, L"CMP_JPEG");
	SingleImageInfo info4 =
	{
		(uint32_t)width,
		(uint32_t)height,
		8,
		tiff::PixelType::PIXEL_UINT8,
		tiff::ImageType::IMAGE_RGB,
		tiff::CompressionMode::COMPRESSIONMODE_JPEG
	};

	CPrecisionTimer timer1(1000);
	CPrecisionTimer timer2(1000);
	CPrecisionTimer timer3(1000);
	CPrecisionTimer timer4(1000);

	long status;
	timer1.StartTimer();
	int hdl1 = open_tiff(single_path1, tiff::OpenMode::CREATE_MODE);
	ASSERT_GE(hdl1, 0);
	long frame_number1 = create_image(hdl1, info1);
	ASSERT_EQ(frame_number1, 0);
	status = save_image_data(hdl1, frame_number1, buf);
	ASSERT_EQ(status, 0);
	close_tiff(hdl1);
	double elapsed1 = timer1.StopTimer();
	ASSERT_EQ(status, 0);

	timer2.StartTimer();
	int hdl2 = open_tiff(single_path2, tiff::OpenMode::CREATE_MODE);
	ASSERT_GE(hdl2, 0);
	long frame_number2 = create_image(hdl2, info2);
	ASSERT_EQ(frame_number2, 0);
	status = save_image_data(hdl2, frame_number2, buf);
	ASSERT_EQ(status, 0);
	close_tiff(hdl2);
	double elapsed2 = timer2.StopTimer();
	ASSERT_EQ(status, 0);

	timer3.StartTimer();
	int hdl3 = open_tiff(single_path3, tiff::OpenMode::CREATE_MODE);
	ASSERT_GE(hdl3, 0);
	long frame_number3 = create_image(hdl3, info2);
	ASSERT_EQ(frame_number3, 0);
	status = save_image_data(hdl3, frame_number3, buf);
	ASSERT_EQ(status, 0);
	close_tiff(hdl3);
	double elapsed3 = timer3.StopTimer();
	ASSERT_EQ(status, 0);

	timer4.StartTimer();
	int hdl4 = open_tiff(single_path4, tiff::OpenMode::CREATE_MODE);
	ASSERT_GE(hdl4, 0);
	long frame_number4 = create_image(hdl4, info2);
	ASSERT_EQ(frame_number4, 0);
	status = save_image_data(hdl4, frame_number4, buf);
	ASSERT_EQ(status, 0);
	close_tiff(hdl4);
	double elapsed4 = timer4.StopTimer();
	ASSERT_EQ(status, 0);

	free(buf);
#ifdef SHOW_TIMER_ELAPSED
	FAIL() << "Elapsed time compare : \n" << "LZW : " << elapsed1 << "\nZIP : " << elapsed2 << "\nRaw : " << elapsed3 << "\nJPEG : " << elapsed4;
#endif // SHOW_TIMER_ELAPSED
}

void continue_save(const wchar_t* name_ext, int count, tiff::CompressionMode compress_mode)
{
	struct _stat64 statbuf;
	int r = _stat64(SRC_PATH, &statbuf);
	ASSERT_FALSE(r || !(statbuf.st_mode & S_IFREG));

	auto fileSize = statbuf.st_size;
	std::unique_ptr<void, std::function<void(void*)>> auto_buffer(malloc(fileSize + !fileSize), free);
	void* const buffer = auto_buffer.get();

	FILE* f;
	fopen_s(&f, SRC_PATH, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, f);
	fclose(f);
	ASSERT_EQ(readSize, fileSize);

	//Decompress Jpeg file to RGB
	int ret = -1;
	tjhandle handle = tjInitDecompress();
	ASSERT_FALSE(handle == nullptr);

	int width, height, subsample, colorspace;
	ret = tjDecompressHeader3(handle, (unsigned char*)buffer, (unsigned long)fileSize, &width, &height, &subsample, &colorspace);
	ASSERT_EQ(ret, 0);

	auto size = tjBufSize(width, height, subsample);
	unsigned char* buf = (unsigned char*)malloc(size);
	memset(buf, 0, size);
	ret = tjDecompress2(handle, (unsigned char*)buffer, (unsigned long)fileSize, buf, width, 0, height, TJPF_RGB, TJFLAG_ACCURATEDCT);
	ASSERT_EQ(ret, 0);

	tjDestroy(handle);

	wchar_t* s_cwd = _wgetcwd(NULL, 0);
	ASSERT_FALSE(s_cwd == NULL);

	CPrecisionTimer timer(1000);
	timer.StartTimer();
	for (int i = 0; i < count; i++)
	{
		wchar_t single_path[256];
		swprintf_s(single_path, 256, L"%s\\test\\%s_MT_%d.tif", s_cwd, name_ext, i);
		SingleImageInfo info =
		{
			(uint32_t)width,
			(uint32_t)height,
			8,
			tiff::PixelType::PIXEL_UINT8,
			tiff::ImageType::IMAGE_RGB,
			compress_mode
		};
		int hdl = open_tiff(single_path, tiff::OpenMode::CREATE_MODE);
		ASSERT_GE(hdl, 0);
		long frame_number = create_image(hdl, info);
		ASSERT_EQ(frame_number, 0);
		long status = save_image_data(hdl, frame_number, buf);
		ASSERT_EQ(status, 0);
		close_tiff(hdl);
	}
	double elapsed = timer.StopTimer();
	free(buf);
#ifdef SHOW_TIMER_ELAPSED
	FAIL() << "Elapsed time : \n" << elapsed;
#endif
}

namespace CLASSIC_SIMPLE_TEST_CASES
{
	TEST(Function_Test, Save_Single_LZW_Compress_RGB_Channels_Frame) { Load_File(L"TIFF_LZW_RGB", tiff::CompressionMode::COMPRESSIONMODE_LZW); }

	TEST(Function_Test, Save_Single_Raw_RGB_Channels_Frame) { Load_File(L"中文TIFF_RAW_RGB", tiff::CompressionMode::COMPRESSIONMODE_NONE); }

	TEST(Function_Test, Save_Single_JPG_Compress_RGB_Channels_Frame) { Load_File(L"測試TIFF_JPG_RGB", tiff::CompressionMode::COMPRESSIONMODE_JPEG); }

	TEST(Function_Test, Save_Single_Frame_RGB_Channels_With_ZIP_Compress) { Load_File(L"TIFF_ZIP_RGB", tiff::CompressionMode::COMPRESSIONMODE_ZIP); }

	//TEST(Function_Test, Save_Load_Single_Frame_RGB_MultiThread) { Save_Load_MultiThread(L"TIFF_RGB", 100, tiff::CompressionMode::COMPRESSIONMODE_LZW); }

	TEST(Function_Test, TEST_Change_Value_Of_Exist_Tag) { Change_Tag(L"TIFF_LZW_TAG_RGB", tiff::CompressionMode::COMPRESSIONMODE_LZW); }

	TEST(Function_Test, Append_Single_JPEG_Compress_RGB_Channels_Frame) { Append_File(L"TIFF_LZW_RGB", tiff::CompressionMode::COMPRESSIONMODE_JPEG); }
}

namespace CLASSIC_PERFORMAANCE_TEST_CASES
{
	TEST(Performance_Test, Continues_Save_JPEG) { continue_save(L"Perf_JPEG_RGB", 100, tiff::CompressionMode::COMPRESSIONMODE_JPEG); }

	TEST(Performance_Test, Continues_Save_LZW) { continue_save(L"Perf_LZW_RGB", 100, tiff::CompressionMode::COMPRESSIONMODE_LZW); }
	TEST(Performance_Test, Continues_Save_ZIP) { continue_save(L"Perf_ZIP_RGB", 100, tiff::CompressionMode::COMPRESSIONMODE_ZIP); }
	TEST(Performance_Test, Continues_Save_RAW) { continue_save(L"Perf_RAW_RGB", 100, tiff::CompressionMode::COMPRESSIONMODE_NONE); }
	TEST(Performance_Test, Compare_ZIP_LZW_Compress_Performance) { compare_performance(); }
}