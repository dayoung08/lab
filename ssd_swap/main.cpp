#include "header.h"
#define NUM_OF_DATEs 30  // for simulation
#define NUM_OF_TIMEs 4 // for simulation

int placement_method = 1;
int migration_method = 1; // 2로 바꾸면 비교스킴
int main() {
	simulation();
	printf("\n[END]\n\n");
	return 0;
}

void simulation() {
	int num_of_SSDs = 20;
	int num_of_videos = 2000000;
	//int video_bandwidth_usage = 1; //비디오가 8000kbps(즉 8Mbps)라고 가정해봅시다. 4*0.125=0.5,하나에 1MB/s가 듭니다.
	int size_of_video = 10; //세그먼트가 10초짜리라고 가정하면, 1MB/s x 10s = 10MB

	SSD* SSD_list = new SSD[num_of_SSDs];
	VIDEO_SEGMENT* VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_videos];

	srand(SEED);
	initalization(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, size_of_video);
	printf("초기화 완료. 이 문구가 빨리 안 뜨면 SSD 숫자를 늘리거나 비디오 세그먼트 수를 줄일 것\n");

	printf("\n[PLACEMENT START]\n\n");
	int placement_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos); // 비디오 하나씩 추가하는 걸로 수정할 것
	//create_placement_infomation(SSD_list, VIDEO_SEGMENT_list); // 배치 정보 파일 생성

	//placement result
	double sum_for_AVG_in_placement = 0;
	double sum_for_STD_in_placement = 0;
	for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
		printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		printf("[SSD %d] DWPD/WAF %.2f\n", ssd, SSD_list[ssd].DWPD);
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
		sum_for_AVG_in_placement += SSD_list[ssd].ADWD;
	}
	for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
		sum_for_STD_in_placement += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_placement / (num_of_SSDs)), 2);
	}

	printf("[Placement] Average ADWD %lf\n", (sum_for_AVG_in_placement / (num_of_SSDs)));
	printf("[Placement] Standard deviation ADWD %lf\n\n", sqrt(sum_for_STD_in_placement / (num_of_SSDs)));

	printf("\n[MIGRATION START]\n\n");

	int* prev_assigned_SSD = new int[num_of_videos];
	double* prev_ADWD = new double[num_of_SSDs];

	double sum_for_AVG_in_migration = 0;
	double sum_for_STD_in_migration = 0;
	double prev_migration_num = 0;
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			if (day == 1 && time == 1)
				continue;
			//cout << time << endl;

			update_video_bandwidth(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, size_of_video);
			for (int vid = 0; vid < num_of_videos; vid++) {
				prev_assigned_SSD[vid] = VIDEO_SEGMENT_list[vid].assigned_SSD;
			}
			migration_num += migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs);
			//create_migration_infomation(SSD_list, VIDEO_SEGMENT_list, prev_assigned_SSD); // 이동 정보 파일 생성

			//migration result
			if (time == NUM_OF_TIMEs) {
				sum_for_AVG_in_migration = 0;
				sum_for_STD_in_migration = 0;
				for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
					double curr_ADWD = ((prev_ADWD[ssd] * (day - 1)) + SSD_list[ssd].ADWD) / day;
					prev_ADWD[ssd] = curr_ADWD;
					sum_for_AVG_in_migration += curr_ADWD;
					SSD_list[ssd].ADWD = 0;
					//printf("[SSD %d] ADWD %.2f\n", ssd, curr_ADWD);
					//printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
					//printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
				}
				for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
					sum_for_STD_in_migration += pow(prev_ADWD[ssd] - (sum_for_AVG_in_migration / (num_of_SSDs)), 2);
				}
				prev_migration_num = ((prev_migration_num * (day - 1)) + migration_num) / day;
			}

		}
		//if (day == 1 || day == 7 || day == 15 || day == 30) {
			//printf("[DAY%d] Average migration number %lf \n", day, prev_migration_num);
		printf("[DAY%d] Average ADWD %lf\n", day, (sum_for_AVG_in_migration / (num_of_SSDs)));
		printf("[DAY%d] Standard deviation ADWD %lf\n\n", day, sqrt(sum_for_STD_in_migration / (num_of_SSDs)));
		//}
	}

	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
	delete[](prev_assigned_SSD);
	delete[](prev_ADWD);
}