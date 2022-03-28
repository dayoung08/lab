#include "header.h"
#define NUM_OF_DATEs 3 // for simulation 1 3 7 15 30
#define NUM_OF_TIMEs 5

#define MIN_RUNNING_DAY 1
#define MAX_RUNNING_DAY 90
//�翬�� �̰� 1�϶��� ���� �� ���� ����....

int placement_method = 1; // 2~6���� �ٲٸ� �񱳽�Ŵ
int migration_method = 7; // 8~11�� �ٲٸ� �񱳽�Ŵ

int num_of_SSDs = 30; // 10, 20, (30), 40, 50
int num_of_videos = 2000000;// 50��, 100��, (150��), 200��, 250��
int num_of_new_videos = 50000; // 10000, 20000, (30000), 40000, 50000 ���� ������ NUM_OF_TIMEs
double num_of_request_per_sec = 20000; //8000

int main(int argc, char* argv[]) {
	//argv �Ķ���Ͱ� ������ �׽�Ʈ ���, ������ �ùķ��̼� ������ ���α׷��� ¥��.
	switch (argc)
	{
	case 1:
		simulation_migartion();
	break;
	case 2:
		if (!strcmp(argv[1], "placement")) {
			testbed_placement();
		}
		else
			printf("command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	case 3:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			num_of_videos = 0;
			num_of_new_videos = 0;
			testbed_migration(stoi(argv[2]));
		}
		else
			printf("command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	case 6:
		if (!strcmp(argv[1], "placement")) {
			num_of_SSDs = stoi(argv[2]);
			num_of_videos = stoi(argv[3]);
			num_of_request_per_sec = stod(argv[4]);
			placement_method = stod(argv[5]);
			simulation_placement();
		}
		else
			printf("command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	case 7:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			num_of_SSDs = stoi(argv[2]);
			num_of_videos = stoi(argv[3]);
			num_of_new_videos = stoi(argv[4]);// / NUM_OF_TIMEs;
			num_of_request_per_sec = stod(argv[5]);
			migration_method = stod(argv[6]);
			simulation_migartion();
		}
		else
			printf("command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	default:
		printf("command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	}
	//printf("\n[END]\n\n");
	return 0;
}

void simulation_placement() {
	std::default_random_engine g(SEED);
	std::uniform_int_distribution<> dist_for_running_day{MIN_RUNNING_DAY, MAX_RUNNING_DAY};
	dist_for_running_day.reset();
	SSD* SSD_list = new SSD[num_of_SSDs + 1]; //SSD_list[num_of_SSDs] -> vitual ssd;
	VIDEO_CHUNK* VIDEO_SEGMENT_list = new VIDEO_CHUNK[num_of_videos];

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
	}
	int total_num = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		total_num += SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.size();
		total_bandwidth_usage_in_placement += SSD_list[ssd].total_bandwidth_usage;
		sum_for_STD_in_placement += pow(SSD_list[ssd].ADWD - (sum_for_AVG_in_placement / num_of_SSDs), 2);
	}
	printf("%lf\n", total_bandwidth_usage_in_placement);

	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
}

void simulation_migartion() {
	std::default_random_engine g(SEED);
	std::uniform_int_distribution<> dist_for_running_day{ MIN_RUNNING_DAY, MAX_RUNNING_DAY };
	dist_for_running_day.reset();
	placement_method = PLACEMENT_RANDOM;
	SSD* SSD_list = new SSD[num_of_SSDs + 1]; //SSD_list[num_of_SSDs] -> vitual ssd;
	VIDEO_CHUNK* VIDEO_SEGMENT_list = new VIDEO_CHUNK[num_of_videos];
	placed_video_init_for_simulation(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);
	//�������� ADWD, running_day, total_write_MB, ���� ���� �Ҵ��� ������ش�.
	placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);

	//�������� ADWD, running_day, total_write_MB, ���� ���� �Ҵ��� ������ش�.
	//int num_new_ssd = 0;
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		SSD_list[ssd].running_days = dist_for_running_day(g);
		SSD_list[ssd].ADWD = 1;
		SSD_list[ssd].total_write_MB = SSD_list[ssd].ADWD * ((SSD_list[ssd].DWPD * SSD_list[ssd].storage_capacity) * SSD_list[ssd].running_days);
	}

	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		int migration_num = 0;
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			//�Ʒ��� ���ο� ���� �߰� ����
			growing_cnt();
			if (num_of_new_videos > 0) {
				// ���ο� ���� �߰��� ���� ���� �������� ������Ʈ ����.
				VIDEO_CHUNK* new_VIDEO_SEGMENT_list = new VIDEO_CHUNK[num_of_new_videos];
				migrated_video_init_for_simulation(SSD_list, VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, num_of_request_per_sec, time);

				VIDEO_CHUNK* _VIDEO_SEGMENT_conbined_list = new VIDEO_CHUNK[num_of_videos + num_of_new_videos];
				copy(VIDEO_SEGMENT_list, VIDEO_SEGMENT_list + num_of_videos, _VIDEO_SEGMENT_conbined_list);
				delete[] VIDEO_SEGMENT_list;
				copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_conbined_list + num_of_videos);
				delete[] new_VIDEO_SEGMENT_list;
				VIDEO_SEGMENT_list = _VIDEO_SEGMENT_conbined_list;
				num_of_videos += num_of_new_videos;  //���� ���� ����Ʈ�� ���ο� ���� �߰�
			}
			else {
				//���ο� ���� ������Ʈ ���ϰ�, �α⵵�� �ٲ� �� ��. 
				migrated_video_init_for_simulation(SSD_list, VIDEO_SEGMENT_list, NULL, migration_method, num_of_SSDs, num_of_videos, 0, num_of_request_per_sec, time);
			}
			//migration ����
			//printf("%d��-%d ", day, time);
			int migration_num;
			if(migration_method >= MIGRATION_OURS)
				migration_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos);
			else 
				migration_num = placement(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos);
			//printf("migration_num %d\n", migration_num);
		}

		// ��� ��� : SSD�� ���, ǥ������ ADWD ���
		if (day == 3) {
		//if (day == 1 || day == 3 || day == 7 || day == 15 || day == 30) {
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
			printf("%lf %lf %lf\n", total_bandwidth_usage_in_migration, sum_for_AVG_in_migration / num_of_SSDs, sqrt(sum_for_STD_in_migration / num_of_SSDs));
		}
	}
/*
	double total_bandwidth_of_alloc_videos = 0;
	int num_of_alloc_videos = 0;
	for (int vid = 0; vid < num_of_videos; vid++) {
		if ( !(VIDEO_SEGMENT_list[vid].assigned_SSD == NONE_ALLOC && VIDEO_SEGMENT_list[vid].assigned_SSD == NONE_ALLOC)) {
			num_of_alloc_videos++;
			total_bandwidth_of_alloc_videos += VIDEO_SEGMENT_list[vid].requested_bandwidth;
		}
		if (VIDEO_SEGMENT_list[vid].assigned_SSD == VIRTUAL_SSD) {
			printf("error\n");
		}
	}
	printf("����� ���� �� ���� %d/%d\n", num_of_alloc_videos, num_of_videos);
	printf("����� ������ Total requested bandwidth %lf / %lf\n", total_bandwidth_of_alloc_videos, ((double)VIDEO_BANDWIDTH * (double)num_of_request_per_sec));
	
	for (int ssd = 1; ssd <= num_of_SSDs; ssd++) {
		printf("[SSD %d] bandwidth usage %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].total_bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].total_bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		printf("[SSD %d] storage %.2f/ %.2f (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_capacity, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_capacity));
		printf("[SSD %d] ADWD %.2f\n", ssd, SSD_list[ssd].ADWD);
	}
	*/
	delete[](SSD_list);
	delete[](VIDEO_SEGMENT_list);
}

void testbed_placement() {
	SSD* SSD_list = NULL;
	VIDEO_CHUNK* VIDEO_SEGMENT_list = NULL;
	placed_video_init_for_testbed(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);

	int placement_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos);

	create_placement_infomation(SSD_list, VIDEO_SEGMENT_list, num_of_videos);
	create_SSD_and_video_list(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos);
}

void testbed_migration(bool _has_new_files) {
	SSD* SSD_list = NULL;
	VIDEO_CHUNK* VIDEO_SEGMENT_list = NULL;
	VIDEO_CHUNK* new_VIDEO_SEGMENT_list = NULL;
	
	// ���ο� ���� �߰��� ���� ���� �������� ������Ʈ ����.
	//���ο� ���� ������Ʈ ���ϰ�, �α⵵�� �ٲٱ� ������
	migrated_video_init_for_testbed(SSD_list, VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, num_of_request_per_sec, _has_new_files);
	int* prev_assigned_SSD = new int[num_of_videos];
	for (int vid = 0; vid < num_of_videos; vid++) {
		prev_assigned_SSD[vid] = VIDEO_SEGMENT_list[vid].assigned_SSD;
	}

	VIDEO_CHUNK* _VIDEO_SEGMENT_conbined_list = new VIDEO_CHUNK[num_of_videos + num_of_new_videos];
	copy(VIDEO_SEGMENT_list, VIDEO_SEGMENT_list + num_of_videos, _VIDEO_SEGMENT_conbined_list);
	delete[] VIDEO_SEGMENT_list;
	copy(new_VIDEO_SEGMENT_list, new_VIDEO_SEGMENT_list + num_of_new_videos, _VIDEO_SEGMENT_conbined_list + num_of_videos);
	delete[] new_VIDEO_SEGMENT_list;
	VIDEO_SEGMENT_list = _VIDEO_SEGMENT_conbined_list;
	num_of_videos += num_of_new_videos;  //���� ���� ����Ʈ�� ���ο� ���� �߰�
	
	//���� migration
	int migration_num;
	if (migration_method >= MIGRATION_OURS)
		migration_num = migration(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos + num_of_new_videos);
	else
		migration_num = placement(SSD_list, VIDEO_SEGMENT_list, placement_method, num_of_SSDs, num_of_videos + num_of_new_videos);

	create_migration_infomation(SSD_list, VIDEO_SEGMENT_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, prev_assigned_SSD); // �̵� ���� ���� ����
	create_SSD_and_video_list(SSD_list, VIDEO_SEGMENT_list, num_of_SSDs, num_of_videos + num_of_new_videos);
	delete[] VIDEO_SEGMENT_list;
}

