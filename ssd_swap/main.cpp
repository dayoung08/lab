#include "header.h"

int main() {
	SSD SSD_list[NUM_OF_SSDs + 1];
	video_VIDEO VIDEO_list[NUM_OF_VIDEOs + 1];
	int METHOD = 2; // 2로 바꾸면 비교스킴
	//#define OUR_METHOD 1
	//#define COMPARATIVE_METHOD 2

	srand(SEED);
	initalization(SSD_list, VIDEO_list);
	printf("초기화 완료. 이 문구가 빨리 안 뜨면 SSD 숫자를 늘리거나 비디오 세그먼트 수를 줄일 것\n");

	//먼저 여기에 SSD의 절반만 다 채워버리자.
	//우란 SSD에 세그먼트들 할당하기.
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
		printf("[SSD %d] DWPD/WAF %.2f\n", ssd, SSD_list[ssd].DWPD);
	}
	printf("\n[START]\n\n");

	double prev_ADWD[NUM_OF_SSDs + 1];
	double total_ADWD = 0;
	double prev_migration_num = 0;
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) { 
			//cout << time << endl;
			update_SSDs_and_insert_new_videos(SSD_list, VIDEO_list);
			migration_num += run(SSD_list, VIDEO_list, METHOD); 
			total_ADWD = 0;
			if (time == NUM_OF_TIMEs) {
				for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
					double curr_ADWD = ((prev_ADWD[ssd] * (day - 1)) + SSD_list[ssd].ADWD) / day;
					prev_ADWD[ssd] = curr_ADWD;
					total_ADWD += curr_ADWD;
					SSD_list[ssd].ADWD = 0;
					//printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
					//printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
				}
				prev_migration_num = ((prev_migration_num * (day - 1)) + migration_num) / day;
				//printf("[DAY%d] ADWD %lf\n", day, (total_for_curr_day / (NUM_OF_SSDs)));
			}
		}
 		printf("[DAY%d] Average migration number %lf \n", day, prev_migration_num);
		printf("[DAY%d] Average ADWD %lf\n\n", day, (total_ADWD / (NUM_OF_SSDs)));
	}

	return 0;
}