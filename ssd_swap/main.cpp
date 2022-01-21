#include "header.h"

int main() {
	SSD SSD_list[NUM_OF_SSDs + 1];
	VIDEO VIDEO_list[NUM_OF_VIDEOs + 1];
	int placement_method = 2;
	int migration_method = 1; // 2로 바꾸면 비교스킴

	srand(SEED);
	initalization(SSD_list, VIDEO_list);
	int placement_num = placement(SSD_list, VIDEO_list, placement_method); // 비디오 하나씩 추가하는 걸로 수정할 것
	printf("%d, 초기화 완료. 이 문구가 빨리 안 뜨면 SSD 숫자를 늘리거나 비디오 세그먼트 수를 줄일 것\n", placement_num);

	//먼저 여기에 SSD의 절반만 다 채워버리자.
	//우란 SSD에 세그먼트들 할당하기.
	double sum_for_AVG2 = 0;
	double sum_for_STD2 = 0;
	double ttt = 0;
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		printf("[SSD %d] DWPD/WAF %.2f\n", ssd, SSD_list[ssd].DWPD);
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
		ttt += SSD_list[ssd].storage_usage;
		sum_for_AVG2 += SSD_list[ssd].ADWD;
	}
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		sum_for_STD2 += pow(SSD_list[ssd].ADWD - (sum_for_AVG2 / (NUM_OF_SSDs)), 2);
	}
	printf("[Placement] %lf\n", ttt);
	printf("[Placement] Average ADWD %lf\n", (sum_for_AVG2 / (NUM_OF_SSDs)));
	printf("[Placement] Standard deviation ADWD %lf\n\n", sqrt(sum_for_STD2 / (NUM_OF_SSDs)));
	printf("\n[START]\n\n");

 	double prev_ADWD[NUM_OF_SSDs + 1];   	double sum_for_AVG = 0;
	double sum_for_STD = 0;
	double prev_migration_num = 0;
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			if (day == 1 && time == 1)
				continue;
			//cout << time << endl;
			update_video_bandwidth(SSD_list, VIDEO_list);
			migration_num += run(SSD_list, VIDEO_list, migration_method);
			if (time == NUM_OF_TIMEs) {
				sum_for_AVG = 0;
				sum_for_STD = 0;
				for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
					double curr_ADWD = ((prev_ADWD[ssd] * (day - 1)) + SSD_list[ssd].ADWD) / day;
					prev_ADWD[ssd] = curr_ADWD;
					sum_for_AVG += curr_ADWD;
					SSD_list[ssd].ADWD = 0;
					//printf("[SSD %d] ADWD %.2f\n", ssd, curr_ADWD);
					//printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
					//printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
				}
				for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
					sum_for_STD += pow(prev_ADWD[ssd] - (sum_for_AVG / (NUM_OF_SSDs)), 2);
				}
				prev_migration_num = ((prev_migration_num * (day - 1)) + migration_num) / day;
			}
		}
		//if (day == 1 || day == 7 || day == 15 || day == 30) {
			//printf("[DAY%d] Average migration number %lf \n", day, prev_migration_num);
			printf("[DAY%d] Average ADWD %lf\n", day, (sum_for_AVG / (NUM_OF_SSDs)));
			printf("[DAY%d] Standard deviation ADWD %lf\n\n", day, sqrt(sum_for_STD / (NUM_OF_SSDs)));
		//}
	}

	return 0;
}