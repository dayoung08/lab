#include "header.h"

int main() {
	SSD SSD_list[NUM_OF_SSDs + 1];
	video_segment segment_list[NUM_OF_SEGMENTs + 1];

	initalization(SSD_list, segment_list);
	printf("�ʱ�ȭ �Ϸ�. �� ������ ���� �� �߸� SSD ���ڸ� �ø��ų� ���� ���׸�Ʈ ���� ���� ��\n");

	//���� ���⿡ SSD�� ���ݸ� �� ä��������.
	//��� SSD�� ���׸�Ʈ�� �Ҵ��ϱ�.
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

		for (int time = 1; time <= NUM_OF_TIMEs; time++) { // 6�ð� �������� ��ũ�ε尡 �ٲ�ٰ� ����.
			cout << time << endl;
			update_SSDs_and_insert_new_videos(SSD_list, segment_list);
			run(SSD_list, segment_list, 1); // 2�� �ٲٸ� �񱳽�Ŵ

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