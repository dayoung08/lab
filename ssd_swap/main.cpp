#include "header.h"

int main() {
	SSD SSD_list[NUM_OF_SSDs + 1];
	video_segment segment_list[NUM_OF_SEGMENTs + 1];
	int METHOD = 2; // 2로 바꾸면 비교스킴

	initalization(SSD_list, segment_list);
	printf("초기화 완료. 이 문구가 빨리 안 뜨면 SSD 숫자를 늘리거나 비디오 세그먼트 수를 줄일 것\n");

	//먼저 여기에 SSD의 절반만 다 채워버리자.
	//우란 SSD에 세그먼트들 할당하기.
	double ADWD_for_prev_day[NUM_OF_SSDs + 1];
	//memset(ADWD_for_prev_day, 0, (sizeof(double) * (NUM_OF_SSDs + 1)));
	double total_ADWD = 0;
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		//printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
		ADWD_for_prev_day[ssd] = SSD_list[ssd].ADWD;
		total_ADWD += ADWD_for_prev_day[ssd];
		printf("[SSD %d]DWPD %.2f\n", ssd, SSD_list[ssd].DWPD);
	}
	printf("[START] Average ADWD %lf\n\n", (total_ADWD / (NUM_OF_SSDs)));

	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		//double ADWD_for_curr_day[NUM_OF_SSDs + 1];
		//memset(ADWD_for_curr_day, 0, (sizeof(double) * (NUM_OF_SSDs + 1)));
		//double total_for_curr_day = 0;
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) { 
			//cout << time << endl;
			update_SSDs_and_insert_new_videos(SSD_list, segment_list);
			migration_num += run(SSD_list, segment_list, METHOD); 

			total_ADWD = 0;

			if (time == NUM_OF_TIMEs) {
				for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
					//ADWD_for_curr_day[ssd] = SSD_list[ssd].ADWD;
					//total_for_curr_day += SSD_list[ssd].ADWD;
					SSD_list[ssd].ADWD = ((ADWD_for_prev_day[ssd] * day) + SSD_list[ssd].ADWD) / (day + 1);
					total_ADWD += SSD_list[ssd].ADWD;
					printf("[SSD %d] %.2f\n", ssd, SSD_list[ssd].ADWD);
					//printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
					//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
				}
				//printf("[DAY%d] ADWD %lf\n", day, (total_for_curr_day / (NUM_OF_SSDs)));
				printf("[DAY%d] Migration number %d\n", day, migration_num);
				printf("[DAY%d] Migration number %d\n", day, migration_num);
			}
		}
		printf("[DAY%d] Average ADWD %lf\n\n", day, (total_ADWD / (NUM_OF_SSDs)));
	}

	return 0;
}