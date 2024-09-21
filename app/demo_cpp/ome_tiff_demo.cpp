// classic_tiff_demo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <filesystem>
#include "..\..\src\ome_tiff\ome_tiff_library.h"

int32_t ome_read_example()
{
	int32_t hdl = ome_open_file(L"ome_read.tif", ome::OpenMode::READ_ONLY_MODE);
	if (hdl < 0)
		return hdl;

	int32_t plate_size = ome_get_plates_num(hdl);
	std::unique_ptr<ome::PlateInfo[]> auto_plate_infos = std::make_unique<ome::PlateInfo[]>(plate_size);
	ome::PlateInfo* plate_infos = auto_plate_infos.get();
	int32_t result = ome_get_plates(hdl, plate_infos);
	if (result != 0)
		return result;

	for (int32_t p = 0; p < plate_size; p++)
	{
		uint32_t plate_id = plate_infos[p].id;

		int32_t well_size = ome_get_wells_num(hdl, plate_id);
		std::unique_ptr<ome::WellInfo[]> auto_well_infos = std::make_unique<ome::WellInfo[]>(well_size);
		ome::WellInfo* well_infos = auto_well_infos.get();
		result = ome_get_wells(hdl, plate_id, well_infos);
		if (result != 0)
			return result;

		int32_t scan_size = ome_get_scans_num(hdl, plate_id);
		std::unique_ptr<ome::ScanInfo[]> auto_scan_infos = std::make_unique<ome::ScanInfo[]>(scan_size);
		ome::ScanInfo* scan_infos = auto_scan_infos.get();
		result = ome_get_scans(hdl, plate_id, scan_infos);
		if (result != 0)
			return result;

		for (int32_t s = 0; s < scan_size; s++)
		{
			ome::ScanInfo scan_info = scan_infos[s];
			uint32_t scan_id = scan_info.id;

			int32_t channel_size = ome_get_channels_num(hdl, plate_id, scan_id);
			std::unique_ptr<ome::ChannelInfo[]> auto_channel_infos = std::make_unique<ome::ChannelInfo[]>(channel_size);
			ome::ChannelInfo* channel_infos = auto_channel_infos.get();
			result = ome_get_channels(hdl, plate_id, scan_id, channel_infos);
			if (result != 0)
				return result;

			for (int32_t w = 0; w < well_size; w++)
			{
				uint32_t well_id = well_infos[w].id;

				int32_t scan_region_size = ome_get_scan_regions_num(hdl, plate_id, scan_id, well_id);
				std::unique_ptr<ome::ScanRegionInfo[]> auto_scan_region_infos = std::make_unique<ome::ScanRegionInfo[]>(scan_region_size);
				ome::ScanRegionInfo* scan_region_infos = auto_scan_region_infos.get();
				result = ome_get_scan_regions(hdl, plate_id, scan_id, well_id, scan_region_infos);
				if (result != 0)
					return result;

				for (int32_t r = 0; r < scan_region_size; r++)
				{
					ome::ScanRegionInfo scan_region_info = scan_region_infos[r];
					uint32_t region_id = scan_region_info.id;

					for (int32_t c = 0; c < channel_size; c++)
					{
						ome::ChannelInfo channel_info = channel_infos[c];
						for (uint32_t t = 0; t < scan_region_info.size_t; t++)
						{
							for (uint32_t z = 0; z < scan_region_info.pixel_size_z; z++)
							{
								ome::FrameInfo frame_info = { 0 };
								frame_info.plate_id = plate_id;
								frame_info.scan_id = scan_id;
								frame_info.region_id = region_id;
								frame_info.c_id = channel_info.id;
								frame_info.t_id = t;
								frame_info.z_id = z;

								ome::OmeRect whole_frame_rect = { 0 };
								whole_frame_rect.x = 0;
								whole_frame_rect.y = 0;
								whole_frame_rect.width = scan_region_info.pixel_size_x;
								whole_frame_rect.height = scan_region_info.pixel_size_y;

								uint32_t image_stride = scan_region_info.pixel_size_x * channel_info.sample_per_pixel * ((scan_info.significant_bits + 7) / 8);

								std::unique_ptr<uint8_t[]> auto_image_data = std::make_unique<uint8_t[]>(image_stride * scan_region_info.pixel_size_y);
								uint8_t* image_data = auto_image_data.get();

								result = ome_get_raw_data(hdl, frame_info, whole_frame_rect, image_data);
								if (result != 0)
									return result;


							}
						}
					}

				}
			}
		}
	}

	ome_close_file(hdl);
	return 0;
}

int32_t ome_write_example()
{
	int32_t hdl_write = ome_open_file(L"ome_write.tif", ome::OpenMode::CREATE_MODE, ome::CompressionMode::COMPRESSIONMODE_LZW);
	if (hdl_write < 0)
		return hdl_write;

	ome::PlateInfo plate_info{};
	plate_info.id = 0;
	plate_info.width = 115.5f;
	plate_info.height = 70.3f;
	plate_info.row_size = 1;
	plate_info.column_size = 2;
	plate_info.physicalsize_unit_x = ome::DistanceUnit::DISTANCE_MILLIMETER;
	plate_info.physicalsize_unit_y = ome::DistanceUnit::DISTANCE_MILLIMETER;
	std::filesystem::path p = std::filesystem::u8path("中文English混合Plate");
	std::wstring plate_name = p.generic_wstring();
	wmemcpy_s(plate_info.name, NAME_LEN, plate_name.c_str(), plate_name.size());
	int32_t result_write = ome_add_plate(hdl_write, plate_info);
	if (result_write != 0)
		return result_write;

	ome::ScanInfo scan_info{};
	scan_info.id = 0;
	scan_info.pixel_physical_size_x = 1.25f;
	scan_info.pixel_physical_size_y = 2.55f;
	scan_info.pixel_physical_size_z = 1.0f;
	scan_info.time_increment = 1.3f;
	scan_info.pixel_physical_uint_x = ome::DistanceUnit::DISTANCE_MICROMETER;
	scan_info.pixel_physical_uint_y = ome::DistanceUnit::DISTANCE_MICROMETER;
	scan_info.pixel_physical_uint_z = ome::DistanceUnit::DISTANCE_MICROMETER;
	scan_info.time_increment_unit = ome::TimeUnit::TIME_SECOND;
	scan_info.tile_pixel_size_width = 256;
	scan_info.tile_pixel_size_height = 256;
	scan_info.significant_bits = 8;
	scan_info.pixel_type = ome::PixelType::PIXEL_UINT8;
	const char* dimension_order = "XYZTC";
	memcpy_s(scan_info.dimension_order, NAME_LEN, dimension_order, strlen(dimension_order));
	result_write = ome_add_scan(hdl_write, plate_info.id, scan_info);
	if (result_write != 0)
		return result_write;

	for (uint32_t w = 0; w < 2; w++)
	{
		ome::WellInfo well_info{};
		well_info.id = w;
		well_info.position_x = 7.55f + ((float)w * 30.1f);
		well_info.position_y = 5.73f;
		well_info.width = 25.5f;
		well_info.height = 18.7f;
		well_info.row_index = 0;
		well_info.column_index = (uint16_t)w;

		result_write = ome_add_well(hdl_write, plate_info.id, well_info);
		if (result_write != 0)
			return result_write;

		ome::ScanRegionInfo scan_region_info{};
		scan_region_info.id = w;
		scan_region_info.pixel_size_x = 512;
		scan_region_info.pixel_size_y = 512;
		scan_region_info.pixel_size_z = 10;
		scan_region_info.size_t = 10;
		scan_region_info.start_physical_x = 8619.1f + (float)w * 32000.0f;
		scan_region_info.start_physical_y = 9828.7f;
		scan_region_info.start_physical_z = 0.0f;
		scan_region_info.start_unit_x = ome::DistanceUnit::DISTANCE_MICROMETER;
		scan_region_info.start_unit_y = ome::DistanceUnit::DISTANCE_MICROMETER;
		scan_region_info.start_unit_z = ome::DistanceUnit::DISTANCE_MICROMETER;

		result_write = ome_add_scan_region(hdl_write, plate_info.id, scan_info.id, well_info.id, scan_region_info);
		if (result_write != 0)
			return result_write;

		for (int32_t c = 0; c < 2; c++)
		{
			ome::ChannelInfo channel_info{};
			channel_info.id = c;
			channel_info.sample_per_pixel = 1;
			std::filesystem::path p_c = std::filesystem::u8path("通道Channel-" + std::to_string(c));
			std::wstring channel_name = p_c.generic_wstring();
			wmemcpy_s(channel_info.name, NAME_LEN, channel_name.c_str(), channel_name.size());
			result_write = ome_add_channel(hdl_write, plate_info.id, scan_info.id, channel_info);
			if (result_write != 0)
				return result_write;

			uint32_t tile_stride = scan_info.tile_pixel_size_width * channel_info.sample_per_pixel * ((scan_info.significant_bits + 7) / 8);
			std::unique_ptr<uint8_t[]> auto_tile_data = std::make_unique<uint8_t[]>(tile_stride * scan_info.tile_pixel_size_height);
			uint8_t* tile_data = auto_tile_data.get();
			memset(tile_data, 0, tile_stride * scan_info.tile_pixel_size_height);

			for (uint32_t i = 0; i < scan_info.tile_pixel_size_height; i += 8)
			{
				uint8_t* ptr = tile_data + i * tile_stride;
				memset(ptr, (c + 1) * (w + 1) * 60, 4 * tile_stride);
			}

			for (uint32_t t = 0; t < scan_region_info.size_t; t++)
			{
				for (uint32_t z = 0; z < scan_region_info.pixel_size_z; z++)
				{
					ome::FrameInfo frame_info = { 0 };
					frame_info.plate_id = plate_info.id;
					frame_info.scan_id = scan_info.id;
					frame_info.region_id = scan_region_info.id;
					frame_info.c_id = channel_info.id;
					frame_info.t_id = t;
					frame_info.z_id = z;

					uint32_t complete_columns = scan_region_info.pixel_size_x / scan_info.tile_pixel_size_width;
					uint32_t complete_rows = scan_region_info.pixel_size_y / scan_info.tile_pixel_size_height;
					uint32_t total_columns = complete_columns;
					uint32_t total_rows = complete_rows;
					if (scan_region_info.pixel_size_x % scan_info.tile_pixel_size_width)
						total_columns += 1;
					if (scan_region_info.pixel_size_y % scan_info.tile_pixel_size_height)
						total_rows += 1;

					for (uint32_t index_column = 0; index_column < total_columns; index_column++)
					{
						uint32_t block_width = scan_info.tile_pixel_size_width;
						if (index_column >= complete_columns)
							block_width = scan_region_info.pixel_size_x - complete_columns * scan_info.tile_pixel_size_width;

						for (uint32_t index_row = 0; index_row < total_rows; index_row++)
						{
							uint32_t block_height = scan_info.tile_pixel_size_height;
							if (index_row >= complete_rows)
								block_height = scan_region_info.pixel_size_y - complete_rows * scan_info.tile_pixel_size_height;

							ome::OmeRect block_rect = { 0 };
							block_rect.x = index_column * scan_info.tile_pixel_size_width;
							block_rect.y = index_row * scan_info.tile_pixel_size_height;
							block_rect.width = block_width;
							block_rect.height = block_height;

							result_write = ome_save_tile_data(hdl_write, tile_data, frame_info, index_row, index_column);
							if (result_write != 0)
								return result_write;
						}
					}
				}
			}
		}
	}

	ome_close_file(hdl_write);

	return 0;
}

int main()
{
	//ome_read_example();
	ome_write_example();
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
