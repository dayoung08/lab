#include "header.h"

int main() {
	SSD SSD_list[NUM_OF_SSDs + 1];
	video_segment segment_list[NUM_OF_SEGMENTs + 1];
	int METHOD = 2; // 2�� �ٲٸ� �񱳽�Ŵ

	initalization(SSD_list, segment_list);
	printf("�ʱ�ȭ �Ϸ�. �� ������ ���� �� �߸� SSD ���ڸ� �ø��ų� ���� ���׸�Ʈ ���� ���� ��\n");

	//���� ���⿡ SSD�� ���ݸ� �� ä��������.
	//��� SSD�� ���׸�Ʈ�� �Ҵ��ϱ�.
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		//printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
		printf("[SSD %d]DWPD %.2f\n", ssd, SSD_list[ssd].DWPD);
	}
	printf("\n[START]\n\n");

	double prev_ADWD[NUM_OF_SSDs + 1];
	double total_ADWD = 0;
	double prev_migration_num = 0;
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		total_ADWD = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) { 
			//cout << time << endl;
			update_SSDs_and_insert_new_videos(SSD_list, segment_list);
			migration_num += run(SSD_list, segment_list, METHOD); 

			if (time == NUM_OF_TIMEs) {
				for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
					SSD_list[ssd].ADWD = ((prev_ADWD[ssd] * (day - 1)) + SSD_list[ssd].ADWD) / day;
					prev_ADWD[ssd] = SSD_list[ssd].ADWD;
					//printf("[SSD %d] %.2f\n", ssd, SSD_list[ssd].ADWD);
					//printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
					//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
					total_ADWD += SSD_list[ssd].ADWD;
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