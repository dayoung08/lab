#include "header.h"
//datanode6이 HDD로 구성되어있다고 가정(실제로는 아님)
void create_placement_infomation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_videos) {
	ofstream fout("placementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기

	if (!fout.fail()) {
		for (int vid = 0; vid < _num_of_videos; vid++) {
			int video_index = vid;
			int ssd_index = _VIDEO_CHUNK_list[video_index].assigned_SSD;

			string line = "";
			line += _VIDEO_CHUNK_list[video_index].path + "\t";

			if (ssd_index <= _num_of_SSDs) {
				line += _SSD_list[ssd_index].node_hostname + "\t";
				/*if (_SSD_list[ssd_index].storage_folder_name == "tlc01")
					line += "0";
				else if (_SSD_list[ssd_index].storage_folder_name == "qlc01")
					line += "1";
				else if (_SSD_list[ssd_index].storage_folder_name == "qlc02")
					line += "2";
				else if (_SSD_list[ssd_index].storage_folder_name == "qlc03")
					line += "3";*/
				line += _SSD_list[ssd_index].storage_folder_name;
			}
			else {
				line += "datanode6\t"; //datanode6이 HDD로 구성되어있다고 가정(실제로는 아님)
				//line += to_string((ssd_index - 1) % 4);
				int temp = (ssd_index - 1) % 4;
				if (temp == 0)
					line += "tlc01";
				if (temp == 1)
					line += "qlc01";
				if (temp == 2)
					line += "qlc02";
				if (temp == 3)
					line += "qlc03";
			}

			if (video_index != _num_of_videos - 1) {
				line += "\n";
			}
			fout << line;   // cout처럼 출력하면 됨.
		}
		fout.close();  // 파일을 닫습니다.
	}
}

void create_migration_infomation(SSD * _SSD_list, VIDEO_CHUNK * _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_existing_videos, int _num_of_new_videos, int* _prev_assigned_SSD) {
	ofstream fout_existing("movementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기
	ofstream fout_new("placementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기

	for (int vid = 0; vid < _num_of_existing_videos + _num_of_new_videos; vid++) {
		int video_index = vid;
		if (video_index < _num_of_existing_videos) { // 기존에 있던 파일

			if (!fout_existing.fail()) {
				int video_index = vid;
				int from_ssd_index = _prev_assigned_SSD[video_index];
				int to_ssd_index = _VIDEO_CHUNK_list[video_index].assigned_SSD;

				if (from_ssd_index != to_ssd_index) { // migration 안 하는 경우는 line 출력 X

					string line = "";
					line += _VIDEO_CHUNK_list[video_index].path + "\t";

					if (from_ssd_index <= _num_of_SSDs) {
						line += _SSD_list[from_ssd_index].node_hostname + "\t";
						/*if (_SSD_list[from_ssd_index].storage_folder_name == "tlc01")
							line += "0\t";
						else if (_SSD_list[from_ssd_index].storage_folder_name == "qlc01")
							line += "1\t";
						else if (_SSD_list[from_ssd_index].storage_folder_name == "qlc02")
							line += "2\t";
						else if (_SSD_list[from_ssd_index].storage_folder_name == "qlc03")
							line += "3\t";*/
						line += _SSD_list[from_ssd_index].storage_folder_name + "\t";
					}
					else {
						line += "datanode6\t"; //datanode6이 HDD로 구성되어있다고 가정(실제로는 아님)
						//line += to_string((from_ssd_index - 1) % 4) + "\t";
						int temp = (from_ssd_index - 1) % 4;
						if (temp == 0)
							line += "tlc01\t";
						if (temp == 1)
							line += "qlc01\t";
						if (temp == 2)
							line += "qlc02\t";
						if (temp == 3)
							line += "qlc03\t";
					}

					if (to_ssd_index <= _num_of_SSDs) {
						line += _SSD_list[to_ssd_index].node_hostname + "\t";
						/*if (_SSD_list[to_ssd_index].storage_folder_name == "tlc01")
							line += "0";
						else if (_SSD_list[to_ssd_index].storage_folder_name == "qlc01")
							line += "1";
						else if (_SSD_list[to_ssd_index].storage_folder_name == "qlc02")
							line += "2";
						else if (_SSD_list[to_ssd_index].storage_folder_name == "qlc03")
							line += "3";*/
						line += _SSD_list[to_ssd_index].storage_folder_name;
					}
					else {
						line += "datanode6\t"; //datanode6이 HDD로 구성되어있다고 가정(실제로는 아님)
						//line += to_string((to_ssd_index - 1) % 4);
						int temp = (to_ssd_index - 1) % 4;
						if (temp == 0)
							line += "tlc01";
						if (temp == 1)
							line += "qlc01";
						if (temp == 2)
							line += "qlc02";
						if (temp == 3)
							line += "qlc03";
					}

					if (video_index != _num_of_existing_videos - 1) {
						line += "\n";
					}

					fout_existing << line;   // cout처럼 출력하면 됨.
				}
			}
		}
		else { // 새로 업로드하는 파일
			//create_placement_infomation랑 거의 똑같은데 함수 만드는게 더 피곤할 것 같아서 그냥 복붙해서 고쳐서 썼다...
			if (!fout_new.fail()) {
				int video_index = vid;
				int ssd_index = _VIDEO_CHUNK_list[video_index].assigned_SSD;

				string line = "";
				line += _VIDEO_CHUNK_list[video_index].path + "\t";

				if (ssd_index <= _num_of_SSDs) {
					line += _SSD_list[ssd_index].node_hostname + "\t";
					/*if (_SSD_list[ssd_index].storage_folder_name == "tlc01")
						line += "0";
					else if (_SSD_list[ssd_index].storage_folder_name == "qlc01")
						line += "1";
					else if (_SSD_list[ssd_index].storage_folder_name == "qlc02")
						line += "2";
					else if (_SSD_list[ssd_index].storage_folder_name == "qlc03")
						line += "3";*/
					line += _SSD_list[ssd_index].storage_folder_name;
				}
				else {
					line += "datanode6\t"; //datanode6이 HDD로 구성되어있다고 가정(실제로는 아님)
					//line += to_string((ssd_index - 1) % 4);
					int temp = (ssd_index - 1) % 4;
					if (temp == 0)
						line += "tlc01";
					if (temp == 1)
						line += "qlc01";
					if (temp == 2)
						line += "qlc02";
					if (temp == 3)
						line += "qlc03";
				}
				if (video_index != _num_of_existing_videos +_num_of_new_videos - 1) {
					line += "\n";
				}

				fout_new << line;   // cout처럼 출력하면 됨.
			}
		}
	}
	fout_existing.close();  // 파일을 닫습니다.
	fout_new.close();  // 파일을 닫습니다.
}

void create_result(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_videos, bool _is_migration) {
	/*ofstream fout_ssd("SSD_list.temp", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기
	if (!fout_ssd.fail()) {
		fout_ssd << to_string(_num_of_SSDs) + "\n";
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
	}*/

	ofstream fout_video("existing_video_list.temp", ios_base::in | ios_base::out | ios_base::trunc);
	if (!fout_video.fail()) {
		if(_is_migration) {
			fout_video << to_string(_num_of_videos) + "\n";
		}
		else{ //placement일 때는, 새 파일들에 대해서만 관리하기 때문에 existing_files를 따로 읽어와줘야 함...
			ifstream fin_existing_video("existing_video_list.in"); // fin 객체 생성(cin 처럼 이용!)
			int num_of_existing_videos;
			int num_of_new_videos = _num_of_videos;
			string str;
			if (!fin_existing_video.fail()) {
				getline(fin_existing_video, str);
				num_of_existing_videos = stoi(str);
			}
			else {
				num_of_existing_videos = 0;
			}

			int total_video_num = num_of_existing_videos + _num_of_videos;
			fout_video << to_string(total_video_num) + "\n";

			if (!fin_existing_video.fail() && num_of_existing_videos > 0) {
				int cnt = 0;
				while (getline(fin_existing_video, str)) // 파일이 끝날때까지 한 줄씩 읽어오기
				{
					if (cnt >= num_of_existing_videos)
						break;
					string* video_info = split(str, '\t');
					string path = video_info[0];
					double size = stod(video_info[1]);
					double once_bandwidth = stod(video_info[2]);
					string datanode = video_info[3];
					string storage_folder_name = video_info[4];

					string line = path + "\t";
					line += to_string(size) + "\t";
					line += to_string(once_bandwidth) + "\t";
					line += datanode + "\t";
					line += storage_folder_name + "\n";
					fout_video << line;
				}
			}
			fin_existing_video.close(); // 파일 닫기
		}

		//migration때는 _VIDEO_CHUNK_list, _num_of_videos 안에 existing video와 new video가 다 담겨 있으므로
		for (int vid = 0; vid < _num_of_videos; vid++) {
			int video_index = vid;
			string line = "";
			line += _VIDEO_CHUNK_list[video_index].path + "\t";
			line += to_string(_VIDEO_CHUNK_list[video_index].size) + "\t";
			line += to_string(_VIDEO_CHUNK_list[video_index].once_bandwidth) + "\t";
			int SSD_type;
			
			int datanode_num = ceil((double)_VIDEO_CHUNK_list[video_index].assigned_SSD / 4);
			line += "datanode" + to_string(datanode_num) + "\t";
			SSD_type = (_VIDEO_CHUNK_list[video_index].assigned_SSD - 1) % 4;

			if (SSD_type == 0)
				line += "tlc01";
			else if (SSD_type == 1)
				line += "qlc01";
			else if (SSD_type == 2)
				line += "qlc02";
			else if (SSD_type == 3)
				line += "qlc03";

			if (vid != _num_of_videos - 1) {
				line += "\n";
			}

			fout_video << line;   // cout처럼 출력하면 됨.
		}
		fout_video.close();  // 파일을 닫습니다.
	}

	remove("existing_video_list.in");
	rename("existing_video_list.temp", "existing_video_list.in");
}