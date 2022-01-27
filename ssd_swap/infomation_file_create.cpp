#include "header.h"

void create_placement_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_new_videos) {
	ofstream fout("placementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기

	if (fout.is_open()) {
		for (int vid = 0; vid < _num_of_new_videos; vid++) {
			int video_index = vid;
			int ssd_index = _new_VIDEO_SEGMENT_list[video_index].assigned_SSD;

			string line = "";
			line += _new_VIDEO_SEGMENT_list[video_index].path + "\t";
			line += _SSD_list[ssd_index].node_hostname + "\t"	+ "0"; // 한 데이터노드 당 하나의 storage만 사용하도록 hadoop 환경을 세팅해 놓을 예정임.

			if (vid != _num_of_new_videos -1) {
				line += "\n";
			}

			fout << line;   // cout처럼 출력하면 됨.
		}
		fout.close();  // 파일을 닫습니다.
	}
}

void create_migration_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, int _num_of_existed_videos, int* _prev_assigned_SSD) {
	ofstream fout("movementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기

	if (fout.is_open()) {
		for (int vid = 0; vid < _num_of_existed_videos; vid++) {
			int video_index = vid;
			int from_ssd_index = _prev_assigned_SSD[video_index];
			int to_ssd_index = _existed_VIDEO_SEGMENT_list[video_index].assigned_SSD;

			if (from_ssd_index == to_ssd_index) { // migration 안 하는 경우는 line 출력 X
				continue;
			}

			string line = "";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].index) + "\t";
			line += _SSD_list[from_ssd_index].node_hostname + "\t" + "DISK" + "\t"; // hadoop 환경에서 storage들 type 지정 안 하면 디폴트가 DISK임
			line += _SSD_list[to_ssd_index].node_hostname + "\t" + "DISK" + "\t" + "0"; // 한 데이터노드 당 하나의 stroage만 사용하도록 hadoop 환경을 세팅해 놓을 예정임.

			if (vid != _num_of_existed_videos - 1) {
				line += "\n";
			}
			fout << line;   // cout처럼 출력하면 됨.
		}
		fout.close();  // 파일을 닫습니다.
	}
}

void create_SSD_and_video_list(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos) {
	ofstream fout_ssd("SSD_list.in", ios_base::in | ios_base::out | ios_base::trunc);   // 파일 열기
	if (fout_ssd.is_open()) {
		for (int ssd = 0; ssd < _num_of_SSDs; ssd++) {
			int ssd_index = ssd;

			string line = "";
			line += _SSD_list[ssd_index].node_hostname + "\t";
			line += to_string(_SSD_list[ssd_index].storage_capacity) + "\t";
			line += to_string(_SSD_list[ssd_index].maximum_bandwidth) + "\t";
			line += to_string(_SSD_list[ssd_index].DWPD) + "\t";
			line += to_string(_SSD_list[ssd_index].WAF) + "\t";
			line += to_string(_SSD_list[ssd_index].storage_usage) + "\t";
			line += to_string(_SSD_list[ssd_index].bandwidth_usage) + "\t";
			line += to_string(_SSD_list[ssd_index].daily_write_MB) + "\t";
			line += to_string(_SSD_list[ssd_index].total_write_MB) + "\t";
			line += to_string(_SSD_list[ssd_index].running_days);

			if (ssd != _num_of_SSDs - 1) {
				line += "\n";
			}
			fout_ssd << line;   // cout처럼 출력하면 됨.
		}
		fout_ssd.close();  // 파일을 닫습니다.
	}

	ofstream fout_video("existed_video_list.in", ios_base::in | ios_base::out | ios_base::trunc);
	if (fout_video.is_open()) {
		for (int vid = 0; vid < _num_of_existed_videos; vid++) {
			int video_index = vid;

			string line = "";
			line += _existed_VIDEO_SEGMENT_list[video_index].path + "\t";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].size) + "\t";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].bandwidth) + "\t";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].assigned_SSD);

			if (vid != _num_of_existed_videos - 1) {
				line += "\n";
			}

			fout_video << line;   // cout처럼 출력하면 됨.
		}
		fout_video.close();  // 파일을 닫습니다.
	}
}