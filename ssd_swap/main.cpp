#include "header.h"
#define NUM_OF_DATEs 15  // for simulation
#define NUM_OF_TIMEs 4 // for simulation

int placement_method = 1; //2,3���� �ٲٸ� �񱳽�Ŵ
int migration_method = 1; // 2�� �ٲٸ� �񱳽�Ŵ
int num_of_SSDs = 30;
int num_of_videos = 1500000;// 50��, 10��, 15��, 20��, 25��, 30��
int num_of_new_videos = 0; //2500, 5000, 7500, 10000, 12500, 15000

int main(int argc, char* argv[]) {
	srand(SEED);
	//argv �Ķ���Ͱ� ������ �׽�Ʈ ���, ������ �ùķ��̼� ������ ���α׷��� ¥��.

	switch (argc)
	{
	case 1:
		simulation();
		break;
	case 2:
		if (!strcmp(argv[1], "placement")) {
			testbed_placement();
		}
		else if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			testbed_migration();
		}
		else
			printf("argv[1] command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	case 6:
		placement_method = stoi(argv[1]);
		migration_method = stoi(argv[2]);
		num_of_SSDs = stoi(argv[3]);
		num_of_videos = stoi(argv[4]);
		num_of_new_videos = stoi(argv[5]);
		simulation();
		break;
	default:
		printf("argc ������ 1�̳�, 2 Ȥ�� 6�̾�� �մϴ�. �ٽ� ������ �ּ���.\n");
		break;
	}
	printf("\n[END]\n\n");
	return 0;
}

void simulation() {
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
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		total_bandwidth_in_placement += SSD_list[ssd].bandwidth_usage;
		sum_for_STD_in_placement += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_placement / num_of_SSDs), 2);
	}
	printf("placement_num %d\n", placement_num);
	printf("[Placement] Total bandwidth usage %lf / %lf\n", total_bandwidth_in_placement, ((double)VIDEO_BANDWIDTH * (double)NUM_OF_REQUEST_PER_SEC) );
	printf("[Placement] �� SSD�� Average ADWD %lf\n", (sum_for_AVG_in_placement / num_of_SSDs));
	printf("[Placement] �� SSD�� Standard deviation ADWD %lf\n", sqrt(sum_for_STD_in_placement / num_of_SSDs));
	
	//ù ���� placement, ���� 31�ϰ� migration (ù ���� placement, migration �� ��)

	printf("\n[MIGRATION START]\n\n");
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			//�Ʒ��� ���ο� ���� �߰� ����
			if (num_of_new_videos > 0) {
				VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_new_videos];
				update_new_video_for_simulation(SSD_list, VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, num_of_new_videos, day);
				// ���ο� ���� �߰��� ���� ���� �������� ������Ʈ ����.
				VIDEO_SEGMENT* _VIDEO_SEGMENT_conbined_list = new VIDEO_SEGMENT[num_of_videos + num_of_new_videos];
				copy(VIDEO_SEGMENT_list, VIDEO_SEGMENT_list + num_of_videos, _VIDEO_SEGMENT_conbined_list);
				delete[] VIDEO_SEGMENT_list;
				copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_conbined_list + num_of_videos);
				delete[] new_VIDEO_SEGMENT_list;
				VIDEO_SEGMENT_list = _VIDEO_SEGMENT_conbined_list;
				num_of_videos += num_of_new_videos;  //���� ���� ����Ʈ�� ���ο� ���� �߰�
			}
			else {
				update_new_video_for_simulation(SSD_list, VIDEO_SEGMENT_list, NULL, num_of_SSDs, num_of_videos, 0, day);
				//���ο� ���� ������Ʈ ���ϰ�, �α⵵�� �ٲ� �� ��. 
			}

			//migration ����
			//printf("%d��-%d ", day, time);
			int migration_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs);
			printf("migration_num %d\n", migration_num);
		}

		// ��� ��� : SSD�� ���, ǥ������ ADWD ���
		//if (day == 1 || day == 7 || day == 15 || day == 30) {
			//printf("[DAY%d] Average migration number %lf \n", day, prev_migration_num);
		double sum_for_AVG_in_migration = 0;
		double sum_for_STD_in_migration = 0;
		double sum_for_DAILY_AVG_in_migration = 0;
		double sum_for_DAILY_STD_in_migration = 0;
		double total_bandwidth_in_migration = 0;
		for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
			double average_ADWD = SSD_list[ssd].total_write_MB / (SSD_list[ssd].storage_capacity * SSD_list[ssd].DWPD) / SSD_list[ssd].running_days;
			sum_for_DAILY_AVG_in_migration += average_ADWD;
			sum_for_AVG_in_migration += SSD_list[ssd].ADWD;
			//printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
			//printf("[SSD %d] storage %.2f/ %.2f (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
			//printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
		}
		for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
			total_bandwidth_in_migration += SSD_list[ssd].bandwidth_usage;
			double average_ADWD = SSD_list[ssd].total_write_MB / (SSD_list[ssd].storage_capacity * SSD_list[ssd].DWPD) / SSD_list[ssd].running_days;
			sum_for_DAILY_STD_in_migration += pow((average_ADWD - (sum_for_DAILY_AVG_in_migration / num_of_SSDs)), 2);
			sum_for_STD_in_migration += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_migration / num_of_SSDs), 2);
		}
		printf("���� Total bandwidth usage %lf / %lf\n", total_bandwidth_in_migration, ((double)VIDEO_BANDWIDTH * (double)NUM_OF_REQUEST_PER_SEC));
		printf("�� SSD�� %d�� ������ Average ADWD %lf\n", day, (sum_for_AVG_in_migration / num_of_SSDs));
		printf("�� SSD�� %d�� ������ Standard deviation ADWD %lf\n\n", day, sqrt(sum_for_STD_in_migration / num_of_SSDs));
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
	printf("����� ���� �� ���� %d/%d\n", num_of_alloc_videos, num_of_videos);
	printf("����� ������ Total requested bandwidth %lf / %lf\n", total_bandwidth_of_alloc_videos, ((double)VIDEO_BANDWIDTH * (double)NUM_OF_REQUEST_PER_SEC));
	
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
	//SSD��, ������ �̹� ����� ���� ����Ʈ�� �� ������

	VIDEO_SEGMENT* new_VIDEO_SEGMENT_list = NULL;
	int num_of_new_videos;
	update_new_video_for_testbed(SSD_list, existed_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, num_of_SSDs, num_of_existed_videos, num_of_new_videos);

	if (new_VIDEO_SEGMENT_list != NULL) {
		int* prev_assigned_SSD = new int[num_of_existed_videos];
		for (int vid = 0; vid < num_of_existed_videos; vid++) {
			prev_assigned_SSD[vid] = existed_VIDEO_SEGMENT_list[vid].assigned_SSD;
		}
		migration(SSD_list, existed_VIDEO_SEGMENT_list, migration_method, num_of_SSDs);
		create_migration_infomation(SSD_list, existed_VIDEO_SEGMENT_list, num_of_existed_videos, prev_assigned_SSD); // �̵� ���� ���� ����
	}

	//placement(SSD_list, existed_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_existed_videos, num_of_new_videos); // ���� �߰��� ������ �Ҵ���.
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

