#include "header.h"
#define NUM_OF_DATEs 7  // for simulation 1, 3, (7), 15, 31
#define NUM_OF_TIMEs 2 

#define MIN_ADWD 10 //1.0
#define MAX_ADWD 50 // 5.0
#define MIN_RUNNING_DAY 30
#define MAX_RUNNING_DAY 90

int placement_method = 1; //2,3으로 바꾸면 비교스킴
int num_of_SSDs = 20; // 10, 15, (20), 25, 30
int num_of_videos = 2000000;// 100만, 150만, (200만), 250만, 300만

int migration_method = MIGRATION_LIFETIME_AWARE; // 2로 바꾸면 비교스킴
int num_of_new_videos = 20000; //1000, 15000, (20000), 25000, 30000

int main(int argc, char* argv[]) {
	srand(SEED);
	//argv 파라미터가 있으면 테스트 배드, 없으면 시뮬레이션 돌리는 프로그램을 짜자.

	switch (argc)
	{
	case 1:
		simulation_placement();
		simulation_migartion();
		break;
	case 2:
		if (!strcmp(argv[1], "placement")) {
			testbed_placement();
		}
		else if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			testbed_migration();
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	case 5:
		if (!strcmp(argv[1], "placement")) {
			placement_method = stoi(argv[2]);
			num_of_SSDs = stoi(argv[3]);
			num_of_videos = stoi(argv[4]);
			simulation_placement();
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	case 6:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			migration_method = stoi(argv[2]);
			num_of_SSDs = stoi(argv[3]);
			num_of_videos = stoi(argv[4]);
			num_of_new_videos = stoi(argv[5]);
			simulation_migartion();
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	default:
		printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	}
	printf("\n[END]\n\n");
	return 0;
}

void simulation_placement() {
	SSD* SSD_list = new SSD[num_of_SSDs + 1]; //SSD_list[num_of_SSDs] -> vitual ssd;
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_videos];

	initalization_for_simulation(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos);

	printf("\n[PLACEMENT START]\n\n");
	int placement_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);
	//int placement_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs);

	double sum_for_AVG_in_placement = 0;
	double sum_for_STD_in_placement = 0;
	double total_bandwidth_in_placement = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		sum_for_AVG_in_placement += SSD_list[ssd].ADWD;
		printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %.2f/ %.2f (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
	}
	int total_num = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		total_num += SSD_list[ssd].assigned_VIDEOs_low_bandwidth_first.size();
		total_bandwidth_in_placement += SSD_list[ssd].bandwidth_usage;
		sum_for_STD_in_placement += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_placement / num_of_SSDs), 2);
	}
	printf("placement_num %d\n", placement_num);
	printf("[Placement] Total bandwidth usage %lf / %lf\n", total_bandwidth_in_placement, ((double)VIDEO_BANDWIDTH * (double)NUM_OF_REQUEST_PER_SEC));
	printf("[Placement] 각 SSD의 Average ADWD %lf\n", (sum_for_AVG_in_placement / num_of_SSDs));
	printf("[Placement] 각 SSD의 Standard deviation ADWD %lf\n", sqrt(sum_for_STD_in_placement / num_of_SSDs));

	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
}

void simulation_migartion() {
	SSD* SSD_list = new SSD[num_of_SSDs + 1]; //SSD_list[num_of_SSDs] -> vitual ssd;
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_videos];
	initalization_for_simulation(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos);
	//랜덤으로 ADWD, running_day, total_write_MB, 현재 비디오 할당을 만들어준다.
	placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);

	int num_new_ssd = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		int binary = rand() % 20;
		if (binary) {
			SSD_list[ssd].ADWD = ((double)(rand() % (MAX_ADWD - MIN_ADWD + 1) + MIN_ADWD)) / 10;
			SSD_list[ssd].running_days = rand() % (MAX_RUNNING_DAY - MIN_RUNNING_DAY + 1) + MIN_RUNNING_DAY;
			SSD_list[ssd].total_write_MB = SSD_list[ssd].ADWD * (SSD_list[ssd].DWPD * SSD_list[ssd].storage_usage) * SSD_list[ssd].running_days;
		}
		else {
			SSD_list[ssd].ADWD = 0;
			SSD_list[ssd].running_days = 0;
			SSD_list[ssd].total_write_MB = 0;
			num_new_ssd++;
		}
	}
	//랜덤으로 ADWD, running_day, total_write_MB, 현재 비디오 할당을 만들어준다.

	printf("\n[MIGRATION START]");
	printf("num_new_ssd : %d\n\n", num_new_ssd);
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			//아래는 새로운 비디오 추가 과정
			if (num_of_new_videos > 0) {
				// 새로운 비디오 추가에 따라 비디오 정보들을 업데이트 해줌.
				VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_new_videos];
				update_new_video_for_simulation(SSD_list, VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, day);
				
				VIDEO_SEGMENT* _VIDEO_SEGMENT_conbined_list = new VIDEO_SEGMENT[num_of_videos + num_of_new_videos];
				copy(VIDEO_SEGMENT_list, VIDEO_SEGMENT_list + num_of_videos, _VIDEO_SEGMENT_conbined_list);
				delete[] VIDEO_SEGMENT_list;
				copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_conbined_list + num_of_videos);
				delete[] new_VIDEO_SEGMENT_list;
				VIDEO_SEGMENT_list = _VIDEO_SEGMENT_conbined_list;
				num_of_videos += num_of_new_videos;  //기존 비디오 리스트에 새로운 비디오 추가
			}
			else {
				//새로운 비디오 업데이트 안하고, 인기도만 바꿀 때 씀. 
				update_new_video_for_simulation(SSD_list, VIDEO_SEGMENT_list, NULL, migration_method, num_of_SSDs, num_of_videos, 0, day);
			}

			//migration 수행
			//printf("%d일-%d ", day, time);
			int migration_num;
			if(migration_method >= MIGRATION_OURS)
				migration_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs);
			else
				migration_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);

			printf("migration_num %d\n", migration_num);
		}

		// 결과 출력 : SSD의 평균, 표준편차 ADWD 출력
		//if (day == 1 || day == 7 || day == 15 || day == 30)
		double sum_for_AVG_in_migration = 0;
		double sum_for_STD_in_migration = 0;
		double total_bandwidth_in_migration = 0;
		for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
			sum_for_AVG_in_migration += SSD_list[ssd].ADWD;
		}
		for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
			total_bandwidth_in_migration += SSD_list[ssd].bandwidth_usage;
			sum_for_STD_in_migration += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_migration / num_of_SSDs), 2);
		}
		printf("현재 Total bandwidth usage %lf / %lf\n", total_bandwidth_in_migration, ((double)VIDEO_BANDWIDTH * (double)NUM_OF_REQUEST_PER_SEC));
		printf("각 SSD의 %d일 동안의 Average ADWD %lf\n", day, (sum_for_AVG_in_migration / num_of_SSDs));
		printf("각 SSD의 %d일 동안의 Standard deviation ADWD %lf\n\n", day, sqrt(sum_for_STD_in_migration / num_of_SSDs));
		//}
	}

	double total_bandwidth_of_alloc_videos = 0;
	int num_of_alloc_videos = 0;
	for (int vid = 0; vid < num_of_videos; vid++) {
		if (VIDEO_SEGMENT_list[vid].assigned_SSD != NONE_ALLOC) {
			num_of_alloc_videos++;
			total_bandwidth_of_alloc_videos += VIDEO_SEGMENT_list[vid].requested_bandwidth;
		}
		if (VIDEO_SEGMENT_list[vid].assigned_SSD == VIRTUAL_SSD) {
			printf("error\n");
		}
	}
	printf("저장된 비디오 총 갯수 %d/%d\n", num_of_alloc_videos, num_of_videos);
	printf("저장된 비디오의 Total requested bandwidth %lf / %lf\n", total_bandwidth_of_alloc_videos, ((double)VIDEO_BANDWIDTH * (double)NUM_OF_REQUEST_PER_SEC));
	
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %.2f/ %.2f (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
	}

	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
}

void testbed_placement() {
	SSD* SSD_list = NULL;
	VIDEO_SEGMENT* existed_VIDEO_SEGMENT_list = NULL;
	int num_of_SSDs, num_of_existed_videos;
	initalization_for_testbed(SSD_list, existed_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos);
	//SSD와, 기존에 이미 저장된 비디오 리스트들 다 가져옴

	VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = NULL;
	int num_of_new_videos;
	update_new_video_for_testbed(SSD_list, existed_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_existed_videos, num_of_new_videos);

	if (new_VIDEO_SEGMENT_list != NULL) {
		int* prev_assigned_SSD = new int[num_of_existed_videos];
		for (int vid = 0; vid < num_of_existed_videos; vid++) {
			prev_assigned_SSD[vid] = existed_VIDEO_SEGMENT_list[vid].assigned_SSD;
		}
		migration(SSD_list, existed_VIDEO_SEGMENT_list, migration_method, num_of_SSDs);
		create_migration_infomation(SSD_list, existed_VIDEO_SEGMENT_list, num_of_existed_videos, prev_assigned_SSD); // 이동 정보 파일 생성
	}

	//placement(SSD_list, existed_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_existed_videos, num_of_new_videos); // 새로 추가한 비디오를 할당함.
	create_placement_infomation(SSD_list, new_VIDEO_SEGMENT_list, num_of_new_videos);
	// 배치 정보 파일 생성

	VIDEO_SEGMENT* _VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_existed_videos + num_of_new_videos];
	copy(existed_VIDEO_SEGMENT_list, existed_VIDEO_SEGMENT_list + num_of_existed_videos, _VIDEO_SEGMENT_list);
	delete[] existed_VIDEO_SEGMENT_list;
	copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_list + num_of_existed_videos + 1);
	delete[] new_VIDEO_SEGMENT_list;
	int num_of_videos = num_of_existed_videos + num_of_new_videos;
	// 기존의 비디오 + 새로운 비디오 리스트를 합침

	create_SSD_and_video_list(SSD_list, existed_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos);
}

void testbed_migration() {
	SSD* SSD_list = NULL;
	VIDEO_SEGMENT* existed_VIDEO_SEGMENT_list = NULL;
	int num_of_SSDs, num_of_existed_videos;
	initalization_for_testbed(SSD_list, existed_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos);
	//SSD와, 기존에 이미 저장된 비디오 리스트들 다 가져옴

	int* prev_assigned_SSD = new int[num_of_existed_videos];
	for (int vid = 0; vid < num_of_existed_videos; vid++) {
		prev_assigned_SSD[vid] = existed_VIDEO_SEGMENT_list[vid].assigned_SSD;
	}
	//비디오 migration
	migration(SSD_list, existed_VIDEO_SEGMENT_list, migration_method, num_of_SSDs);

	create_migration_infomation(SSD_list, existed_VIDEO_SEGMENT_list, num_of_existed_videos, prev_assigned_SSD); // 이동 정보 파일 생성
}

