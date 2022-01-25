#include "header.h"
#define NUM_OF_DATEs 30  // for simulation
#define NUM_OF_TIMEs 4 // for simulation

int placement_method = 1; //2,3으로 바꾸면 비교스킴
int migration_method = 1; // 2로 바꾸면 비교스킴
int main(int argc, char* argv[]) {
	srand(SEED);
	//argv 파라미터가 있으면 테스트 배드, 없으면 시뮬레이션 돌리는 프로그램을 짜자.

	switch (argc)
	{
	case 1:
		simulation();
		break;
	case 2:
		if (!strcmp(argv[1], "placement")) {
			SSD* SSD_list = NULL;
			VIDEO_SEGMENT* VIDEO_SEGMENT_existed_list = NULL;
			int num_of_SSDs, num_of_existed_videos;
			initalization_for_testbed(SSD_list, VIDEO_SEGMENT_existed_list, num_of_SSDs, num_of_existed_videos);

			VIDEO_SEGMENT* VIDEO_SEGMENT_new_list = NULL;
			int num_of_new_videos;
			update_new_video_for_testbed(SSD_list, VIDEO_SEGMENT_existed_list, VIDEO_SEGMENT_new_list, num_of_SSDs, num_of_existed_videos, num_of_new_videos);

			placement(SSD_list, VIDEO_SEGMENT_new_list, placement_method, num_of_SSDs, num_of_new_videos); // 새로 추가한 비디오를 할당함.
			create_placement_infomation(SSD_list, VIDEO_SEGMENT_existed_list, num_of_new_videos); // 배치 정보 파일 생성
		}
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			SSD* SSD_list = NULL;
			VIDEO_SEGMENT* VIDEO_SEGMENT_existed_list = NULL;
			int num_of_SSDs, num_of_existed_videos;
		
			initalization_for_testbed(SSD_list, VIDEO_SEGMENT_existed_list, num_of_SSDs, num_of_existed_videos);
			
			int* prev_assigned_SSD = new int[num_of_existed_videos];
			for (int vid = 0; vid < num_of_existed_videos; vid++) {
				prev_assigned_SSD[vid] = VIDEO_SEGMENT_existed_list[vid].assigned_SSD;
			}
			//비디오 migration
			migration(SSD_list, VIDEO_SEGMENT_existed_list, migration_method, num_of_SSDs);
			create_migration_infomation(SSD_list, VIDEO_SEGMENT_existed_list, prev_assigned_SSD, num_of_existed_videos); // 이동 정보 파일 생성
		}
	default:
		simulation();
		break;
	}
	printf("\n[END]\n\n");
	return 0;
}

void simulation() {
	int num_of_SSDs = 20;
	int num_of_videos = 1700000;

	SSD* SSD_list = new SSD[num_of_SSDs];
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_videos];

	initalization_for_simulation(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos);

	printf("\n[PLACEMENT START]\n\n");
	placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos); // 비디오 하나씩 추가하는 걸로 수정할 것
	//create_placement_infomation(SSD_list, VIDEO_SEGMENT_list); // 배치 정보 파일 생성

	double sum_for_AVG_in_placement = 0;
	double sum_for_STD_in_placement = 0;
	for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
		sum_for_AVG_in_placement += SSD_list[ssd].ADWD;
		printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		printf("[SSD %d] DWPD/WAF %.2f\n", ssd, SSD_list[ssd].DWPD);
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
	}
	for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
		sum_for_STD_in_placement += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_placement / num_of_SSDs), 2);
	}
	printf("[Placement] 각 SSD의 Average ADWD %lf\n", (sum_for_AVG_in_placement / num_of_SSDs));
	printf("[Placement] 각 SSD의 Standard deviation ADWD %lf\n\n", sqrt(sum_for_STD_in_placement / num_of_SSDs));


	printf("\n[MIGRATION START]\n\n");
	//double sum_for_AVG_in_migration = 0;
	//double sum_for_STD_in_migration = 0;

	double* sum_ADWD = new double[num_of_SSDs];
	fill(sum_ADWD, sum_ADWD + num_of_SSDs, 0);
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			//cout << time << endl;
			int* prev_assigned_SSD = new int[num_of_videos];
			for (int vid = 0; vid < num_of_videos; vid++) {
				prev_assigned_SSD[vid] = VIDEO_SEGMENT_list[vid].assigned_SSD;
			}

			int num_of_existed_videos = num_of_videos;
			int num_of_new_videos = 2500;
			VIDEO_SEGMENT* VIDEO_SEGMENT_new_list = new VIDEO_SEGMENT[num_of_new_videos];
			update_new_video_for_simulation(SSD_list, VIDEO_SEGMENT_list, VIDEO_SEGMENT_new_list, num_of_SSDs, num_of_existed_videos, num_of_new_videos); // 새로운 비디오 추가에 따라 비디오 정보들을 업데이트 해줌.

			//아래는 새로운 비디오 추가 과정
			placement(SSD_list, VIDEO_SEGMENT_new_list, placement_method, num_of_SSDs, num_of_new_videos); // 새로 추가한 비디오를 할당함.
			//비디오 migration
			migration_num += migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs);
			//create_migration_infomation(SSD_list, VIDEO_SEGMENT_list, prev_assigned_SSD); // 이동 정보 파일 생성
			delete[] prev_assigned_SSD;

			VIDEO_SEGMENT* _VIDEO_SEGMENT_conbined_list = new VIDEO_SEGMENT[num_of_existed_videos + num_of_new_videos];
			copy(VIDEO_SEGMENT_list, VIDEO_SEGMENT_list + num_of_existed_videos, _VIDEO_SEGMENT_conbined_list);
			delete[] VIDEO_SEGMENT_list;
			copy(VIDEO_SEGMENT_new_list, VIDEO_SEGMENT_new_list + num_of_new_videos, _VIDEO_SEGMENT_conbined_list + num_of_existed_videos + 1);
			delete[] VIDEO_SEGMENT_new_list;
			VIDEO_SEGMENT_list = _VIDEO_SEGMENT_conbined_list;
			num_of_videos = num_of_existed_videos + num_of_new_videos; // 기존의 비디오 + 새로운 비디오 리스트를 합침

			//migration result 출력
			if (time == NUM_OF_TIMEs) {
				//sum_for_AVG_in_migration = 0;
				//sum_for_STD_in_migration = 0;
				for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
					//sum_for_AVG_in_migration += SSD_list[ssd].ADWD;
					sum_ADWD[ssd] += SSD_list[ssd].ADWD;

					SSD_list[ssd].ADWD = 0;
					SSD_list[ssd].write_MB = 0;
					//printf("[SSD %d] ADWD %.2f\n", ssd, curr_ADWD);
					//printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
					//printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
				}
				//for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
				//	sum_for_STD_in_migration += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_migration / num_of_SSDs), 2);
				//}
			}
		}
		//if (day == 1 || day == 7 || day == 15 || day == 30) {
			//printf("[DAY%d] Average migration number %lf \n", day, prev_migration_num);
		double sum_for_DAILY_AVG_in_migration = 0;
		double sum_for_DAILY_STD_in_migration = 0;
		for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
			sum_for_DAILY_AVG_in_migration += sum_ADWD[ssd];
		}
		for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
			sum_for_DAILY_STD_in_migration += pow(sum_ADWD[ssd] - (sum_for_DAILY_AVG_in_migration / num_of_SSDs), 2);
		}
		printf("각 SSD의 %d일 동안의 Average ADWD %lf\n", day, (sum_for_DAILY_AVG_in_migration / num_of_SSDs / day));
		printf("각 SSD의 %d일 동안의 Standard deviation ADWD %lf\n", day, sqrt(sum_for_DAILY_STD_in_migration / num_of_SSDs / day));
		//}
	}

	delete[](sum_ADWD);
	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
}