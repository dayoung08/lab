#include "header.h"
#define NUM_OF_DATEs 1// for simulation 1 3 7 15 30
#define NUM_OF_TIMEs 4

#define MIN_RUNNING_DAY 1
#define MAX_RUNNING_DAY 30
//당연히 이거 1일때가 제일 잘 나옴 으앙....

int placement_method = 1; // 2~6으로 바꾸면 비교스킴
int migration_method = 7; // 8~11로 바꾸면 비교스킴

int num_of_SSDs = 30; // 10, 20, (30), 40, 50
int num_of_videos = 3000000;// 100만, 200만, (300만), 400만, 500만
int num_of_new_videos = 0; // 10000, 20000, (30000), 40000, 50000 에서 나누기 NUM_OF_TIMEs
double num_of_request_per_sec = 25000;

vector<double> result1;
vector<double> result2;
vector<double> result3;

vector<double> result1_day1;
vector<double> result2_day1;
vector<double> result3_day1;

vector<double> result1_day7;
vector<double> result2_day7;
vector<double> result3_day7;

vector<double> result1_day15;
vector<double> result2_day15;
vector<double> result3_day15;

vector<double> result1_day30;
vector<double> result2_day30;
vector<double> result3_day30;


int main(int argc, char* argv[]) {
	//argv 파라미터가 있으면 테스트 배드, 없으면 시뮬레이션 돌리는 프로그램을 짜자.
	switch (argc)
	{
	case 1:
		migration_method = 7;
		simulation_migartion();
		migration_method = 9;
		simulation_migartion();

		int cnt;
		cnt = 0;
		while (cnt < result2.size()) {
			printf("%lf\n", result2[cnt++]);
		}
		printf("\n");
		result2.clear();
		cnt = 0;
		while (cnt < result3.size()) {
			printf("%lf\n", result3[cnt++]);
		}
		printf("\n");
		result3.clear();
		cnt = 0;
		while (cnt < result1.size()) {
			printf("%lf\n", result1[cnt++]);
		}
		printf("\n");
		result1.clear();


		if (NUM_OF_DATEs != 3) {
			cnt = 0;
			while (cnt < result2_day1.size()) {
				printf("%lf\n", result2_day1[cnt++]);
			}
			printf("\n");
			result2_day1.clear();
			cnt = 0;
			while (cnt < result3_day1.size()) {
				printf("%lf\n", result3_day1[cnt++]);
			}
			printf("\n");
			result3_day1.clear();
			cnt = 0;
			while (cnt < result1_day1.size()) {
				printf("%lf\n", result1_day1[cnt++]);
			}
			printf("\n");
			result1_day1.clear();

			cnt = 0;
			while (cnt < result2_day7.size()) {
				printf("%lf\n", result2_day7[cnt++]);
			}
			printf("\n");
			result2_day7.clear();
			cnt = 0;
			while (cnt < result3_day7.size()) {
				printf("%lf\n", result3_day7[cnt++]);
			}
			printf("\n");
			result3_day7.clear();
			cnt = 0;
			while (cnt < result1_day7.size()) {
				printf("%lf\n", result1_day7[cnt++]);
			}
			printf("\n");
			result1_day7.clear();

			cnt = 0;
			while (cnt < result2_day15.size()) {
				printf("%lf\n", result2_day15[cnt++]);
			}
			printf("\n");
			result2_day15.clear();
			cnt = 0;
			while (cnt < result3_day15.size()) {
				printf("%lf\n", result3_day15[cnt++]);
			}
			printf("\n");
			result3_day15.clear();
			cnt = 0;
			while (cnt < result1_day15.size()) {
				printf("%lf\n", result1_day15[cnt++]);
			}
			printf("\n");
			result1_day15.clear();

			cnt = 0;
			while (cnt < result2_day30.size()) {
				printf("%lf\n", result2_day30[cnt++]);
			}
			printf("\n");
			result2_day30.clear();
			cnt = 0;
			while (cnt < result3_day30.size()) {
				printf("%lf\n", result3_day30[cnt++]);
			}
			printf("\n");
			result3_day30.clear();
			cnt = 0;
			while (cnt < result1_day30.size()) {
				printf("%lf\n", result1_day30[cnt++]);
			}
			printf("\n");
			result1_day30.clear();
		}
		break;
	/*for (int i = 1; i < MIGRATION_OURS; i++) {
		placement_method = i;
		simulation_placement();
	}*/
	break;
	case 2:
		if (!strcmp(argv[1], "placement")) {
			testbed_placement();
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	case 3:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			num_of_videos = 0;
			num_of_new_videos = 0;
			testbed_migration(stoi(argv[2]));
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	case 5:
		if (!strcmp(argv[1], "placement")) {
			num_of_SSDs = stoi(argv[2]);
			num_of_videos = stoi(argv[3]);
			num_of_request_per_sec = stod(argv[4]);
			for (int i = 1; i < MIGRATION_OURS; i++) {
				placement_method = i;
				simulation_placement();
			}
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	case 6:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			num_of_SSDs = stoi(argv[2]);
			num_of_videos = stoi(argv[3]);
			num_of_new_videos = stoi(argv[4]) / NUM_OF_TIMEs;
			num_of_request_per_sec = stod(argv[5]);

			for (int i = MIGRATION_OURS; i < MIGRATION_LIFETIME_AWARE + 1; i++) {
				migration_method = i;
				simulation_migartion();
			}
			migration_method = 1;
			simulation_migartion();

			int cnt;
			cnt = 0;
			while (cnt < result2.size()) {
				printf("%lf\n", result2[cnt++]);
			}
			printf("\n");
			result2.clear();
			cnt = 0;
			while (cnt < result3.size()) {
				printf("%lf\n", result3[cnt++]);
			}
			printf("\n");
			result3.clear();
			cnt = 0;
			while (cnt < result1.size()) {
				printf("%lf\n", result1[cnt++]);
			}
			printf("\n");
			result1.clear();


			if (NUM_OF_DATEs != 3) {
				cnt = 0;
				while (cnt < result2_day1.size()) {
					printf("%lf\n", result2_day1[cnt++]);
				}
				printf("\n");
				result2_day1.clear();
				cnt = 0;
				while (cnt < result3_day1.size()) {
					printf("%lf\n", result3_day1[cnt++]);
				}
				printf("\n");
				result3_day1.clear();
				cnt = 0;
				while (cnt < result1_day1.size()) {
					printf("%lf\n", result1_day1[cnt++]);
				}
				printf("\n");
				result1_day1.clear();

				cnt = 0;
				while (cnt < result2_day7.size()) {
					printf("%lf\n", result2_day7[cnt++]);
				}
				printf("\n");
				result2_day7.clear();
				cnt = 0;
				while (cnt < result3_day7.size()) {
					printf("%lf\n", result3_day7[cnt++]);
				}
				printf("\n");
				result3_day7.clear();
				cnt = 0;
				while (cnt < result1_day7.size()) {
					printf("%lf\n", result1_day7[cnt++]);
				}
				printf("\n");
				result1_day7.clear();

				cnt = 0;
				while (cnt < result2_day15.size()) {
					printf("%lf\n", result2_day15[cnt++]);
				}
				printf("\n");
				result2_day15.clear();
				cnt = 0;
				while (cnt < result3_day15.size()) {
					printf("%lf\n", result3_day15[cnt++]);
				}
				printf("\n");
				result3_day15.clear();
				cnt = 0;
				while (cnt < result1_day15.size()) {
					printf("%lf\n", result1_day15[cnt++]);
				}
				printf("\n");
				result1_day15.clear();

				cnt = 0;
				while (cnt < result2_day30.size()) {
					printf("%lf\n", result2_day30[cnt++]);
				}
				printf("\n");
				result2_day30.clear();
				cnt = 0;
				while (cnt < result3_day30.size()) {
					printf("%lf\n", result3_day30[cnt++]);
				}
				printf("\n");
				result3_day30.clear();
				cnt = 0;
				while (cnt < result1_day30.size()) {
					printf("%lf\n", result1_day30[cnt++]);
				}
				printf("\n");
				result1_day30.clear();
			}
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	default:
		printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	}
	//printf("\n[END]\n\n");
	return 0;
}

void simulation_placement() {
	std::mt19937 g(SEED);
	std::uniform_int_distribution<> dist_for_running_day{MIN_RUNNING_DAY, MAX_RUNNING_DAY};
	SSD* SSD_list = new SSD[num_of_SSDs + 1]; //SSD_list[num_of_SSDs] -> vitual ssd;
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_videos];

	placed_video_init_for_simulation(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		SSD_list[ssd].running_days = dist_for_running_day(g);
		SSD_list[ssd].ADWD = 1;
		SSD_list[ssd].total_write_MB = SSD_list[ssd].ADWD * ((SSD_list[ssd].DWPD * SSD_list[ssd].storage_capacity) * SSD_list[ssd].running_days);
	}

	//printf("\n[PLACEMENT START]\n\n");
	int placement_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);
	//int placement_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs);

	double sum_for_AVG_in_placement = 0;
	double sum_for_STD_in_placement = 0;
	//double total_serviced_bandwidth_in_placement = 0;
	double total_bandwidth_usage_in_placement = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		sum_for_AVG_in_placement += SSD_list[ssd].ADWD; 
		//printf("[SSD %d] serviced bandwidth %.2f / %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].serviced_bandwidth_usage, SSD_list[ssd].total_bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].serviced_bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD %d] storage %.2f/ %.2f (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		//printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
	}
	int total_num = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		total_num += SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.size();
		total_bandwidth_usage_in_placement += SSD_list[ssd].total_bandwidth_usage;
		sum_for_STD_in_placement += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_placement / num_of_SSDs), 2);
	}
	//printf("placement_num %d\n", placement_num);
	//printf("현재 Total serviced bandwidth usage %lf / %lf\n", total_serviced_bandwidth_in_placement, ((double)VIDEO_BANDWIDTH * (double)num_of_request_per_sec));
	//printf("[Placement] 각 SSD의 Average ADWD %lf\n", (sum_for_AVG_in_placement / num_of_SSDs));
	//printf("[Placement] 각 SSD의 Standard deviation ADWD %lf\n", sqrt(sum_for_STD_in_placement / num_of_SSDs));
	printf("%lf\n", total_bandwidth_usage_in_placement);

	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
}

void simulation_migartion() {
	std::mt19937 g(SEED);
	std::uniform_int_distribution<> dist_for_running_day{ MIN_RUNNING_DAY, MAX_RUNNING_DAY };
	placement_method = PLACEMENT_RANDOM;
	SSD* SSD_list = new SSD[num_of_SSDs + 1]; //SSD_list[num_of_SSDs] -> vitual ssd;
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_videos];
	placed_video_init_for_simulation(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);
	//랜덤으로 ADWD, running_day, total_write_MB, 현재 비디오 할당을 만들어준다.
	placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);

	//랜덤으로 ADWD, running_day, total_write_MB, 현재 비디오 할당을 만들어준다.
	//int num_new_ssd = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		SSD_list[ssd].running_days = dist_for_running_day(g);
		SSD_list[ssd].ADWD = 1;
		SSD_list[ssd].total_write_MB = SSD_list[ssd].ADWD * ((SSD_list[ssd].DWPD * SSD_list[ssd].storage_capacity) * SSD_list[ssd].running_days);
	}

	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			cout << day << "-" << time << endl;
			//아래는 새로운 비디오 추가 과정
			if (num_of_new_videos > 0) {
				// 새로운 비디오 추가에 따라 비디오 정보들을 업데이트 해줌.
				VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_new_videos];
				growing_cnt();
				migrated_video_init_for_simulation(SSD_list, VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, num_of_request_per_sec, day);

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
				growing_cnt();
				migrated_video_init_for_simulation(SSD_list, VIDEO_SEGMENT_list, NULL, migration_method, num_of_SSDs, num_of_videos, 0, num_of_request_per_sec, day);
			}
			//migration 수행
			//printf("%d일-%d ", day, time);
			int migration_num;
			if(migration_method >= MIGRATION_OURS)
				migration_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos);
			else {
				migration_num = placement(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos);
			}

			//printf("migration_num %d\n", migration_num);
		}

		// 결과 출력 : SSD의 평균, 표준편차 ADWD 출력
		//if (day == 3) {
		if (day == 1 || day == 3 || day == 7 || day == 15 || day == 30) {
			double sum_for_AVG_in_migration = 0;
			double sum_for_STD_in_migration = 0;
			//double total_serviced_bandwidth_in_migration = 0;
			double total_bandwidth_usage_in_migration = 0;
			for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
				sum_for_AVG_in_migration += SSD_list[ssd].ADWD;
			}
			for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
				total_bandwidth_usage_in_migration += SSD_list[ssd].total_bandwidth_usage;
				sum_for_STD_in_migration += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_migration / num_of_SSDs), 2);
			}
			/*printf("현재 Total serviced bandwidth usage %lf / %lf\n", total_bandwidth_usage_in_migration, ((double)VIDEO_BANDWIDTH * (double)num_of_request_per_sec));
			printf("각 SSD의 %d일 동안의 Average ADWD %lf\n", day, (sum_for_AVG_in_migration / num_of_SSDs));
			printf("각 SSD의 %d일 동안의 Standard deviation ADWD %lf\n\n", day, sqrt(sum_for_STD_in_migration / num_of_SSDs));*/
			if (day == 3) {
				result1.push_back(total_bandwidth_usage_in_migration);
				result2.push_back((sum_for_AVG_in_migration / num_of_SSDs));
				result3.push_back(sqrt(sum_for_STD_in_migration / num_of_SSDs));
			}
			if (day == 1) {
				result1_day1.push_back(total_bandwidth_usage_in_migration);
				result2_day1.push_back((sum_for_AVG_in_migration / num_of_SSDs));
				result3_day1.push_back(sqrt(sum_for_STD_in_migration / num_of_SSDs));
			}
			if (day == 7) {
				result1_day7.push_back(total_bandwidth_usage_in_migration);
				result2_day7.push_back((sum_for_AVG_in_migration / num_of_SSDs));
				result3_day7.push_back(sqrt(sum_for_STD_in_migration / num_of_SSDs));
			}
			if (day == 15) {
				result1_day15.push_back(total_bandwidth_usage_in_migration);
				result2_day15.push_back((sum_for_AVG_in_migration / num_of_SSDs));
				result3_day15.push_back(sqrt(sum_for_STD_in_migration / num_of_SSDs));
			}
			if (day == 30) {
				result1_day30.push_back(total_bandwidth_usage_in_migration);
				result2_day30.push_back((sum_for_AVG_in_migration / num_of_SSDs));
				result3_day30.push_back(sqrt(sum_for_STD_in_migration / num_of_SSDs));
			}

		}
	}

	/*double total_bandwidth_of_alloc_videos = 0;
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
	printf("저장된 비디오의 Total requested bandwidth %lf / %lf\n", total_bandwidth_of_alloc_videos, ((double)VIDEO_BANDWIDTH * (double)num_of_request_per_sec));
	
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		printf("[SSD %d] bandwidth usage %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].total_bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].total_bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %.2f/ %.2f (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
	}*/

	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
}

void testbed_placement() {
	SSD* SSD_list = NULL;
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = NULL;
	placed_video_init_for_testbed(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);

	int placement_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);

	create_placement_infomation(SSD_list, VIDEO_SEGMENT_list, num_of_videos);
	create_SSD_and_video_list(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos);
}

void testbed_migration(bool _has_new_files) {
	SSD* SSD_list = NULL;
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = NULL;
	VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = NULL;
	
	// 새로운 비디오 추가에 따라 비디오 정보들을 업데이트 해줌.
	//새로운 비디오 업데이트 안하고, 인기도만 바꾸기 가능함
	migrated_video_init_for_testbed(SSD_list, VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, num_of_request_per_sec, _has_new_files);
	int* prev_assigned_SSD = new int[num_of_videos];
	for (int vid = 0; vid < num_of_videos; vid++) {
		prev_assigned_SSD[vid] = VIDEO_SEGMENT_list[vid].assigned_SSD;
	}

	VIDEO_SEGMENT* _VIDEO_SEGMENT_conbined_list = new VIDEO_SEGMENT[num_of_videos + num_of_new_videos];
	copy(VIDEO_SEGMENT_list, VIDEO_SEGMENT_list + num_of_videos, _VIDEO_SEGMENT_conbined_list);
	delete[] VIDEO_SEGMENT_list;
	copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_conbined_list + num_of_videos);
	delete[] new_VIDEO_SEGMENT_list;
	VIDEO_SEGMENT_list = _VIDEO_SEGMENT_conbined_list;
	num_of_videos += num_of_new_videos;  //기존 비디오 리스트에 새로운 비디오 추가
	
	//비디오 migration
	int migration_num;
	if (migration_method >= MIGRATION_OURS)
		migration_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos + num_of_new_videos);
	else
		migration_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos + num_of_new_videos);

	create_migration_infomation(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, prev_assigned_SSD); // 이동 정보 파일 생성
	create_SSD_and_video_list(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos + num_of_new_videos);
	delete[] VIDEO_SEGMENT_list;
}

