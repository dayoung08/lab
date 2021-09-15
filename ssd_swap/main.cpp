#include "header.h"

int main() {
	SSD SSD_list[NUM_OF_SSDs + 1];
	video_segment segment_list[NUM_OF_SEGMENTs + 1];

	initalization(SSD_list, segment_list);
	printf("초기화 완료. 이 문구가 빨리 안 뜨면 SSD 숫자를 늘리거나 비디오 세그먼트 수를 줄일 것\n");

	//먼저 여기에 SSD의 절반만 다 채워버리자.
	//우란 SSD에 세그먼트들 할당하기.
	double ADWD_for_prev_day[NUM_OF_SSDs + 1];
	memset(ADWD_for_prev_day, 0, (sizeof(double) * (NUM_OF_SSDs + 1)));
	double total_ADWD = 0;
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		//printf("[SSD %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
		total_ADWD += ADWD_for_prev_day[ssd];
	}
	//printf("[START] %lf\n", (total_ADWD / (NUM_OF_SSDs)));

	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		double ADWD_for_curr_day[NUM_OF_SSDs + 1];
		memset(ADWD_for_curr_day, 0, (sizeof(double) * (NUM_OF_SSDs + 1)));

		for (int time = 1; time <= NUM_OF_TIMEs; time++) { // 6시간 간격으로 워크로드가 바뀐다고 가정.
			cout << time << endl;
			update_SSDs_and_insert_new_videos(SSD_list, segment_list);
			run(SSD_list, segment_list, 1); // 2로 바꾸면 비교스킴

			total_ADWD = 0;
			for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
				if (time == NUM_OF_TIMEs) {
					ADWD_for_curr_day[ssd] = SSD_list[ssd].ADWD;
					SSD_list[ssd].ADWD = ((ADWD_for_prev_day[ssd] * day) + SSD_list[ssd].ADWD) / (day + 1);
					total_ADWD += SSD_list[ssd].ADWD;
					//printf("[SSD %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
					//printf("[SSD %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
				}
			}
		}
		printf("[DAY%d] %lf\n", day, (total_ADWD / (NUM_OF_SSDs)));
	}

	return 0;
}