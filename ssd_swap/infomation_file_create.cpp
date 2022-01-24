#include "header.h"
#include <fstream>     

void create_placement_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list) {
	ofstream fout;        // ���� ��� ��ü ����
	fout.open("placementInfo.in");   // ���� ����

	for (int vid = 0; vid < NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		int ssd_index = _VIDEO_SEGMENT_list[video_index].assigned_SSD;

		string line = "";
		line += _VIDEO_SEGMENT_list[video_index].name + "\t";
		line += _SSD_list[ssd_index].node_hostname + "\t"	+ "0"; // �� �����ͳ�� �� �ϳ��� stroage�� ����ϵ��� hadoop ȯ���� ������ ���� ������.

		if (vid != NUM_OF_VIDEOs-1) {
			line += "\n";
		}

		fout << line;   // coutó�� ����ϸ� ��.
	}

	if (fout.is_open()) {
		fout.close();  // ������ �ݽ��ϴ�.
	}
}

void create_migration_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int* _prev_assigned_SSD) {
	ofstream fout;        // ���� ��� ��ü ����
	fout.open("movementInfo.in");   // ���� ����

	for (int vid = 0; vid < NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		int from_ssd_index = _prev_assigned_SSD[video_index];
		int to_ssd_index = _VIDEO_SEGMENT_list[video_index].assigned_SSD;

		string line = "";
		line += to_string(_VIDEO_SEGMENT_list[video_index].index) + "\t";
		line += _SSD_list[from_ssd_index].node_hostname + "\t" + "DISK" + "\t"; // hadoop ȯ�濡�� storage�� type ���� �� �ϸ� ����Ʈ�� DISK��
		line += _SSD_list[to_ssd_index].node_hostname + "\t" + "DISK" + "\t" + "0"; // �� �����ͳ�� �� �ϳ��� stroage�� ����ϵ��� hadoop ȯ���� ������ ���� ������.

		if (vid != NUM_OF_VIDEOs - 1) {
			line += "\n";
		}
		fout << line;   // coutó�� ����ϸ� ��.
	}

	if (fout.is_open()) {
		fout.close();  // ������ �ݽ��ϴ�.
	}
}