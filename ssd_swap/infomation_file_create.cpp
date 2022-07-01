#include "header.h"

void create_placement_infomation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_videos) {
	ofstream fout("placementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기

	if (fout.is_open()) {
		for (int vid = 0; vid < _num_of_videos; vid++) {
			int video_index = vid;
			int ssd_index = _VIDEO_CHUNK_list[video_index].assigned_SSD;

			string line = "";
			line += _VIDEO_CHUNK_list[video_index].path + "\t";
			line += _SSD_list[ssd_index].node_hostname + "\t";
			if (_SSD_list[ssd_index].storage_folder_name == "tlc01")
				line += "0";
			else if (_SSD_list[ssd_index].storage_folder_name == "qlc01")
				line += "1";
			else if (_SSD_list[ssd_index].storage_folder_name == "qlc02")
				line += "2";
			else if (_SSD_list[ssd_index].storage_folder_name == "qlc03")
				line += "3";

			if (video_index != _num_of_videos - 1) {
				line += "\n";
			}
			fout << line;   // cout처럼 출력하면 됨.
		}
		fout.close();  // 파일을 닫습니다.
	}
}

void create_migration_infomation(SSD * _SSD_list, VIDEO_CHUNK * _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_existing_videos, int _num_of_new_videos, int* _prev_assigned_SSD) {
	for (int vid = 0; vid < _num_of_existing_videos + _num_of_new_videos; vid++) {
		int video_index = vid;
		if (video_index < _num_of_existing_videos) { // 기존에 있던 파일
			ofstream fout("movementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기

			if (fout.is_open()) {
				int video_index = vid;
				int from_ssd_index = _prev_assigned_SSD[video_index];
				int to_ssd_index = _VIDEO_CHUNK_list[video_index].assigned_SSD;

				if (from_ssd_index == to_ssd_index) { // migration 안 하는 경우는 line 출력 X
					continue;
				}

				string line = "";
				line += _VIDEO_CHUNK_list[video_index].path + "\t";
				line += _SSD_list[from_ssd_index].node_hostname + "\t";
				if (_SSD_list[from_ssd_index].storage_folder_name == "tlc01")
					line += "0\t";
				else if (_SSD_list[from_ssd_index].storage_folder_name == "qlc01")
					line += "1\t";
				else if (_SSD_list[from_ssd_index].storage_folder_name == "qlc02")
					line += "2\t";
				else if (_SSD_list[from_ssd_index].storage_folder_name == "qlc03")
					line += "3\t";

				line += _SSD_list[to_ssd_index].node_hostname + "\t";
				if (_SSD_list[to_ssd_index].storage_folder_name == "tlc01")
					line += "0";
				else if (_SSD_list[to_ssd_index].storage_folder_name == "qlc01")
					line += "1";
				else if (_SSD_list[to_ssd_index].storage_folder_name == "qlc02")
					line += "2";
				else if (_SSD_list[to_ssd_index].storage_folder_name == "qlc03")
					line += "3";

				if (video_index != _num_of_existing_videos - 1) {
					line += "\n";
				}
				fout << line;   // cout처럼 출력하면 됨.
			}
			fout.close();  // 파일을 닫습니다.
		}
		else { // 새로 업로드하는 파일
			//create_placement_infomation랑 거의 똑같은데 함수 만드는게 더 피곤할 것 같아서 그냥 복붙해서 고쳐서 썼다...
			ofstream fout("placementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기
			if (fout.is_open()) {
				int video_index = vid;
				int ssd_index = _VIDEO_CHUNK_list[video_index].assigned_SSD;

				string line = "";
				line += _VIDEO_CHUNK_list[video_index].path + "\t";
				line += _SSD_list[ssd_index].node_hostname + "\t";

				if (_SSD_list[ssd_index].storage_folder_name == "tlc01")
					line += "0";
				else if (_SSD_list[ssd_index].storage_folder_name == "qlc01")
					line += "1";
				else if (_SSD_list[ssd_index].storage_folder_name == "qlc02")
					line += "2";
				else if (_SSD_list[ssd_index].storage_folder_name == "qlc03")
					line += "3";

				if (video_index != _num_of_existing_videos +_num_of_new_videos - 1) {
					line += "\n";
				}

				fout << line;   // cout처럼 출력하면 됨.
			}
			fout.close();  // 파일을 닫습니다.
		}
	}
}

void create_SSD_and_video_list(SSD* _SSD_list, VIDEO_CHUNK* _existed_VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_existing_videos) {
	ofstream fout_ssd("SSD_list.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기
	if (fout_ssd.is_open()) {
		for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
			int ssd_index = ssd;

			string line = "";
			line += _SSD_list[ssd_index].node_hostname + "\t";
			line += _SSD_list[ssd_index].storage_folder_name + "\t";
			line += to_string(_SSD_list[ssd_index].storage_capacity) + "\t";
			line += to_string(_SSD_list[ssd_index].maximum_bandwidth) + "\t";
			line += to_string(_SSD_list[ssd_index].DWPD) + "\t";
			line += to_string(_SSD_list[ssd_index].total_write) + "\t";
			line += to_string(_SSD_list[ssd_index].age);

			if (ssd != _num_of_SSDs) {
				line += "\n";
			}
			fout_ssd << line;   // cout처럼 출력하면 됨.
		}
		fout_ssd.close();  // 파일을 닫습니다.
	}

	ofstream fout_video("existed_video_list.in", ios_base::in | ios_base::out | ios_base::trunc);
	if (fout_video.is_open()) {
		for (int vid = 0; vid < _num_of_existing_videos; vid++) {
			int video_index = vid;

			string line = "";
			line += _existed_VIDEO_CHUNK_list[video_index].path + "\t";
			line += to_string(_existed_VIDEO_CHUNK_list[video_index].size) + "\t";
			line += to_string(_existed_VIDEO_CHUNK_list[video_index].once_bandwidth) + "\t";

			int datanode_num = ceil((_existed_VIDEO_CHUNK_list[video_index].assigned_SSD - 1) / 4);
			int SSD_type = (_existed_VIDEO_CHUNK_list[video_index].assigned_SSD - 1) % 4;
			line += "datanode" + to_string(datanode_num) + "\t";
			if(SSD_type == 0)
				line += "tlc01";
			else if (SSD_type == 1)
				line += "qlc01";
			else if (SSD_type == 2)
				line += "qlc02";
			else if (SSD_type == 3)
				line += "qlc03";

			if (vid != _num_of_existing_videos - 1) {
				line += "\n";
			}

			fout_video << line;   // cout처럼 출력하면 됨.
		}
		fout_video.close();  // 파일을 닫습니다.
	}
}