#include "header.h"
#define NUM_OF_DATEs 31  // for simulation
#define NUM_OF_TIMEs 4 // for simulation

int placement_method = 1; //2,3���� �ٲٸ� �񱳽�Ŵ
int migration_method = 2; // 2�� �ٲٸ� �񱳽�Ŵ
int main(int argc, char* argv[]) {
	srand(SEED);
	//argv �Ķ���Ͱ� ������ �׽�Ʈ ���, ������ �ùķ��̼� ������ ���α׷��� ¥��.

	switch (argc)
	{
	case 1:
		simulation();
		break;
	case 2:
		if (!strcmp(argv[1], "simulation")) {
			simulation();
		}
		else if (!strcmp(argv[1], "placement")) {
			testbed_placement();
		}
		else if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			testbed_migration();
		}
		else
			printf("argv[1] command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	default:
		printf("argc ������ 1�̳� 2���� �մϴ�. �ٽ� ������ �ּ���.\n");
		break;
	}
	printf("\n[END]\n\n");
	return 0;
}

void testbed_placement() {
	SSD* SSD_list = NULL;
	VIDEO_SEGMENT* existed_VIDEO_SEGMENT_list = NULL;
	int num_of_SSDs, num_of_existed_videos;
	initalization_for_testbed(SSD_list, existed_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos);
	//SSD��, ������ �̹� ����� ���� ����Ʈ�� �� ������

	VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = NULL;
	int num_of_new_videos;
	update_new_video_for_testbed(SSD_list, existed_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos, num_of_new_videos);
	placement(SSD_list, new_VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_new_videos); // ���� �߰��� ������ �Ҵ���.
	create_placement_infomation(SSD_list, new_VIDEO_SEGMENT_list, num_of_new_videos);
	// ��ġ ���� ���� ����

	VIDEO_SEGMENT* _VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_existed_videos + num_of_new_videos];
	copy(existed_VIDEO_SEGMENT_list, existed_VIDEO_SEGMENT_list + num_of_existed_videos, _VIDEO_SEGMENT_list);
	delete[] existed_VIDEO_SEGMENT_list;
	copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_list + num_of_existed_videos + 1);
	delete[] new_VIDEO_SEGMENT_list;
	int num_of_videos = num_of_existed_videos + num_of_new_videos;
	// ������ ���� + ���ο� ���� ����Ʈ�� ��ħ

	create_SSD_and_video_list(SSD_list, existed_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos);
}

void testbed_migration() {
	SSD* SSD_list = NULL;
	VIDEO_SEGMENT* existed_VIDEO_SEGMENT_list = NULL;
	int num_of_SSDs, num_of_existed_videos;
	initalization_for_testbed(SSD_list, existed_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos);
	//SSD��, ������ �̹� ����� ���� ����Ʈ�� �� ������

	int* prev_assigned_SSD = new int[num_of_existed_videos];
	for (int vid = 0; vid < num_of_existed_videos; vid++) {
		prev_assigned_SSD[vid] = existed_VIDEO_SEGMENT_list[vid].assigned_SSD;
	}
	//���� migration
	migration(SSD_list, existed_VIDEO_SEGMENT_list, migration_method, num_of_SSDs);

	create_migration_infomation(SSD_list, existed_VIDEO_SEGMENT_list, num_of_existed_videos, prev_assigned_SSD); // �̵� ���� ���� ����
}

void simulation() {
	int num_of_SSDs = 20;
	int num_of_existed_videos = 1400000;

	SSD* SSD_list = new SSD[num_of_SSDs];
	VIDEO_SEGMENT* existed_VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_existed_videos];

	initalization_for_simulation(SSD_list, existed_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos);

	printf("\n[PLACEMENT START]\n\n");
	placement(SSD_list, existed_VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_existed_videos); // ���� �ϳ��� �߰��ϴ� �ɷ� ������ ��
	//create_placement_infomation(SSD_list, VIDEO_SEGMENT_list); // ��ġ ���� ���� ����

	double sum_for_AVG_in_placement = 0;
	double sum_for_STD_in_placement = 0;

	double total_bandwidth_in_placement = 0;
	for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
		sum_for_AVG_in_placement += SSD_list[ssd].ADWD;
		printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %.2f/ %.2f (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		//printf("[SSD %d] DWPD/WAF %.2f\n", ssd, SSD_list[ssd].DWPD);
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
	}
	for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
		total_bandwidth_in_placement += SSD_list[ssd].bandwidth_usage;
		sum_for_STD_in_placement += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_placement / num_of_SSDs), 2);
	}
	printf("[Placement] Total bandwidth usage %lf / %lf\n", total_bandwidth_in_placement, 37500.0f);
	printf("[Placement] �� SSD�� Average ADWD %lf\n", (sum_for_AVG_in_placement / num_of_SSDs));
	printf("[Placement] �� SSD�� Standard deviation ADWD %lf\n", sqrt(sum_for_STD_in_placement / num_of_SSDs));
	printf("1���� �Ϸ�\n");

	printf("\n[MIGRATION START]\n\n");
	for (int day = 2; day <= NUM_OF_DATEs; day++) {
		for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
			//SSD_list[ssd].ADWD = 0;
			//SSD_list[ssd].daily_write_MB = 0;
			SSD_list[ssd].running_days = day;
		} // ��� SSD�� ���� Ÿ�� ����
		int migration_num = 0;

		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			//cout << time << endl;
			int* prev_assigned_SSD = new int[num_of_existed_videos];
			for (int vid = 0; vid < num_of_existed_videos; vid++) {
				prev_assigned_SSD[vid] = existed_VIDEO_SEGMENT_list[vid].assigned_SSD;
			}
			int num_of_new_videos = 5000;

			//�Ʒ��� ���ο� ���� �߰� ����
			VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_new_videos];
			update_new_video_for_simulation(SSD_list, existed_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos, num_of_new_videos); // ���ο� ���� �߰��� ���� ���� �������� ������Ʈ ����.
			placement(SSD_list, new_VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_new_videos); // ���� �߰��� ������ �Ҵ���.

			//���� migration
			migration_num += migration(SSD_list, existed_VIDEO_SEGMENT_list, migration_method, num_of_SSDs);
			//create_migration_infomation(SSD_list, existed_VIDEO_SEGMENT_list, num_of_existed_videos, prev_assigned_SSD); // �̵� ���� ���� ����
			delete[] prev_assigned_SSD;

			VIDEO_SEGMENT* _VIDEO_SEGMENT_conbined_list = new VIDEO_SEGMENT[num_of_existed_videos + num_of_new_videos];
			copy(existed_VIDEO_SEGMENT_list, existed_VIDEO_SEGMENT_list + num_of_existed_videos, _VIDEO_SEGMENT_conbined_list);
			delete[] existed_VIDEO_SEGMENT_list;
			copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_conbined_list + num_of_existed_videos);
			delete[] new_VIDEO_SEGMENT_list;
			existed_VIDEO_SEGMENT_list = _VIDEO_SEGMENT_conbined_list;

			num_of_existed_videos += num_of_new_videos; // ������ ���� + ���ο� ���� ����Ʈ�� ��ħ

		}

		// ��� ��� : SSD�� ���, ǥ������ ADWD ���
		//if (day == 1 || day == 7 || day == 15 || day == 30) {
			//printf("[DAY%d] Average migration number %lf \n", day, prev_migration_num);
		double sum_for_DAILY_AVG_in_migration = 0;
		double sum_for_DAILY_STD_in_migration = 0;
		double total_bandwidth_in_migration = 0;
		for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
			double average_ADWD = SSD_list[ssd].total_write_MB / (SSD_list[ssd].storage_capacity * SSD_list[ssd].DWPD) / day;
			sum_for_DAILY_AVG_in_migration += average_ADWD;
		}
		for (int ssd = 0; ssd < num_of_SSDs; ssd++) {
			total_bandwidth_in_migration += SSD_list[ssd].bandwidth_usage;
			double average_ADWD = SSD_list[ssd].total_write_MB / (SSD_list[ssd].storage_capacity * SSD_list[ssd].DWPD) / day;
			sum_for_DAILY_STD_in_migration += pow((average_ADWD - (sum_for_DAILY_AVG_in_migration / num_of_SSDs)), 2);
		}
		printf("���� Total bandwidth usage %lf / %lf\n", total_bandwidth_in_migration, 37500.0f);
		printf("�� SSD�� %d�� ������ Average ADWD %lf\n", day, (sum_for_DAILY_AVG_in_migration / num_of_SSDs));
		printf("�� SSD�� %d�� ������ Standard deviation ADWD %lf\n\n", day, sqrt(sum_for_DAILY_STD_in_migration / num_of_SSDs));
		//}
	}
	delete[](SSD_list);
	delete[](existed_VIDEO_SEGMENT_list);
}