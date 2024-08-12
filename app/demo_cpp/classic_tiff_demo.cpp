// classic_tiff_demo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "..\..\src\classic_tiff\classic_tiff_library.h"

void tiff_read_example()
{
    int32_t hdl = open_tiff(L"classic_tiff_read_sample.tif", tiff::OpenMode::READ_ONLY_MODE);
    if (hdl < 0)
    {
        std::cout << "Open file failed with error : " << hdl << std::endl;
        return;
    }
    do
    {
        uint32_t image_count = 0;
        int32_t status = get_image_count(hdl, &image_count);
        if (status != 0)
        {
            std::cout << "Get image count in file failed." << std::endl;
            break;
        }
        tiff::SingleImageInfo info;
        status = get_image_info(hdl, 0, &info);
        if (status != 0)
        {
            std::cout << "Get image info with index 0 in file failed." << std::endl;
            break;
        }
        uint8_t* buffer = (uint8_t*)calloc(info.width * info.height * info.samples_per_pixel, ((info.valid_bits + 7) / 8));
        if (buffer == nullptr)
        {
            std::cout << "Alloc buffer failed." << std::endl;
            break;
        }
        status = load_image_data(hdl, 0, buffer);
        if (status != 0)
            std::cout << "tiff_read_example failed." << std::endl;
        else {
            std::cout << "tiff_read_example success." << std::endl;
        }
        free(buffer);
    } while (false);

    close_tiff(hdl);
}

void tiff_read_performance()
{

}

void tiff_write_example()
{
    int32_t hdl = open_tiff(L"classic_tiff_write_example.tif", tiff::OpenMode::CREATE_MODE);
    if (hdl < 0)
    {
        std::cout << "Create file failed with error : " << hdl << std::endl;
        return;
    }

    do
    {
        tiff::SingleImageInfo info;
        info.width = 512;
        info.height = 512;
        info.compress_mode = tiff::CompressionMode::COMPRESSIONMODE_NONE;
        info.image_type = tiff::ImageType::IMAGE_GRAY;
        info.pixel_type = tiff::PixelType::PIXEL_UINT16;
        info.valid_bits = 16;
        info.samples_per_pixel = 1;
        int32_t image_number = create_image(hdl, info);
        if (image_number < 0)
        {
            std::cout << "Create image failed with error : " << image_number << std::endl;
            break;
        }

        uint16_t* buffer = (uint16_t*)calloc(info.width * info.height, 2);
        if (buffer == nullptr)
        {
            std::cout << "Alloc buffer failed." << std::endl;
            break;
        }

        for (int i = 0; i < (int)(info.height * info.width); i++)
        {
            uint16_t value = i % (65536);
			buffer[i] = value;
        }

        int32_t status = save_image_data(hdl, image_number, buffer);
        if (status != 0)
            std::cout << "tiff_write_example failed." << std::endl;
        else {
            std::cout << "tiff_write_example success." << std::endl;
        }

        free(buffer);
    } while (false);

    close_tiff(hdl);
}

void tiff_write_performance()
{

}

int main()
{
    tiff_write_example();
    tiff_read_example();
    std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
