#include "header.h"
#include <fstream>     

void create_placement_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list) {
	ofstream fout;        // 파일 출력 객체 생성
	fout.open("placementInfo.in");   // 파일 열기

	for (int vid = 0; vid < NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		int ssd_index = _VIDEO_SEGMENT_list[video_index].assigned_SSD;

		string line = "";
		line += _VIDEO_SEGMENT_list[video_index].name + "\t";
		line += _SSD_list[ssd_index].node_hostname + "\t"	+ "0"; // 한 데이터노드 당 하나의 stroage만 사용하도록 hadoop 환경을 세팅해 놓을 예정임.

		if (vid != NUM_OF_VIDEOs-1) {
			line += "\n";
		}

		fout << line;   // cout처럼 출력하면 됨.
	}

	if (fout.is_open()) {
		fout.close();  // 파일을 닫습니다.
	}
}

void create_migration_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int* _prev_assigned_SSD) {
	ofstream fout;        // 파일 출력 객체 생성
	fout.open("movementInfo.in");   // 파일 열기

	for (int vid = 0; vid < NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		int from_ssd_index = _prev_assigned_SSD[video_index];
		int to_ssd_index = _VIDEO_SEGMENT_list[video_index].assigned_SSD;

		string line = "";
		line += to_string(_VIDEO_SEGMENT_list[video_index].index) + "\t";
		line += _SSD_list[from_ssd_index].node_hostname + "\t" + "DISK" + "\t"; // hadoop 환경에서 storage들 type 지정 안 하면 디폴트가 DISK임
		line += _SSD_list[to_ssd_index].node_hostname + "\t" + "DISK" + "\t" + "0"; // 한 데이터노드 당 하나의 stroage만 사용하도록 hadoop 환경을 세팅해 놓을 예정임.

		if (vid != NUM_OF_VIDEOs - 1) {
			line += "\n";
		}
		fout << line;   // cout처럼 출력하면 됨.
	}

	if (fout.is_open()) {
		fout.close();  // 파일을 닫습니다.
	}
}