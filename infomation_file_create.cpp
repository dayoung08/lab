#include "header.h"

void create_placement_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_new_videos) {
	ofstream fout("placementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // ���� ����

	if (fout.is_open()) {
		for (int vid = 0; vid < _num_of_new_videos; vid++) {
			int video_index = vid;
			int ssd_index = _new_VIDEO_SEGMENT_list[video_index].assigned_SSD;

			string line = "";
			line += _new_VIDEO_SEGMENT_list[video_index].path + "\t";
			line += _SSD_list[ssd_index].node_hostname + "\t"	+ "0"; // �� �����ͳ�� �� �ϳ��� storage�� ����ϵ��� hadoop ȯ���� ������ ���� ������.

			if (vid != _num_of_new_videos -1) {
				line += "\n";
			}

			fout << line;   // coutó�� ����ϸ� ��.
		}
		fout.close();  // ������ �ݽ��ϴ�.
	}
}

void create_migration_infomation(SSD * _SSD_list, VIDEO_SEGMENT * _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos, int* _prev_assigned_SSD) {
	
	for (int vid = 0; vid < _num_of_existed_videos + _num_of_new_videos; vid++) {
		int video_index = vid;
		if (video_index < _num_of_existed_videos) { // ������ �ִ� ����
			ofstream fout("movementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // ���� ����

			if (fout.is_open()) {
				int video_index = vid;
				int from_ssd_index = _prev_assigned_SSD[video_index];
				int to_ssd_index = _VIDEO_SEGMENT_list[video_index].assigned_SSD;

				if (from_ssd_index == to_ssd_index) { // migration �� �ϴ� ���� line ��� X
					continue;
				}

				string line = "";
				line += to_string(_VIDEO_SEGMENT_list[video_index].index) + "\t";
				line += _SSD_list[from_ssd_index].node_hostname + "\t" + "DISK" + "\t"; // hadoop ȯ�濡�� storage�� type ���� �� �ϸ� ����Ʈ�� DISK��
				line += _SSD_list[to_ssd_index].node_hostname + "\t" + "DISK" + "\t" + "0"; // �� �����ͳ�� �� �ϳ��� stroage�� ����ϵ��� hadoop ȯ���� ������ ���� ������.

				if (vid != _num_of_existed_videos - 1) {
					line += "\n";
				}
				fout << line;   // coutó�� ����ϸ� ��.
			}
			fout.close();  // ������ �ݽ��ϴ�.
		}
		else { // ���� ���ε��ϴ� ����
			//create_placement_infomation�� ���� �Ȱ����� �Լ� ����°� �� �ǰ��� �� ���Ƽ� �׳� �����ؼ� ���ļ� ���...
			ofstream fout("placementInfo.in", ios_base::in | ios_base::out | ios_base::trunc);   // ���� ����
			if (fout.is_open()) {
				int video_index = vid;
				int ssd_index = _VIDEO_SEGMENT_list[video_index].assigned_SSD;

				string line = "";
				line += _VIDEO_SEGMENT_list[video_index].path + "\t";
				line += _SSD_list[ssd_index].node_hostname + "\t" + "0"; // �� �����ͳ�� �� �ϳ��� storage�� ����ϵ��� hadoop ȯ���� ������ ���� ������.

				if (vid != _num_of_existed_videos +_num_of_new_videos - 1) {
					line += "\n";
				}

				fout << line;   // coutó�� ����ϸ� ��.
			}
			fout.close();  // ������ �ݽ��ϴ�.
		}
	}
}

void create_SSD_and_video_list(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos) {
	ofstream fout_ssd("SSD_list.in", ios_base::in | ios_base::out | ios_base::trunc);   // ���� ����
	if (fout_ssd.is_open()) {
		for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
			int ssd_index = ssd;

			string line = "";
			line += _SSD_list[ssd_index].node_hostname + "\t";
			line += to_string(_SSD_list[ssd_index].storage_capacity) + "\t";
			line += to_string(_SSD_list[ssd_index].maximum_bandwidth) + "\t";
			line += to_string(_SSD_list[ssd_index].DWPD) + "\t";
			line += to_string(_SSD_list[ssd_index].total_write_MB) + "\t";
			line += to_string(_SSD_list[ssd_index].running_days);

			if (ssd != _num_of_SSDs - 1) {
				line += "\n";
			}
			fout_ssd << line;   // coutó�� ����ϸ� ��.
		}
		fout_ssd.close();  // ������ �ݽ��ϴ�.
	}

	ofstream fout_video("existed_video_list.in", ios_base::in | ios_base::out | ios_base::trunc);
	if (fout_video.is_open()) {
		for (int vid = 0; vid < _num_of_existed_videos; vid++) {
			int video_index = vid;

			string line = "";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].type) + "\t";
			line += _existed_VIDEO_SEGMENT_list[video_index].path + "\t";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].size) + "\t";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].once_bandwidth) + "\t";
			line += to_string(_existed_VIDEO_SEGMENT_list[video_index].assigned_SSD);
			//_existed_VIDEO_SEGMENT_list[video_index].is_serviced

			if (vid != _num_of_existed_videos - 1) {
				line += "\n";
			}

			fout_video << line;   // coutó�� ����ϸ� ��.
		}
		fout_video.close();  // ������ �ݽ��ϴ�.
	}
}