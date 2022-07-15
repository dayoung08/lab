//testbed���� placement���� new_video_list.in�� �־����. existing_video_list.in�� ����� �� ������ �ƴ� ���·� ������ infomation_file_create.cpp���� ������ �߻���.
//���� testbed���� migration ��, existing_video_list.in�� datanode�� � SSD�� ����Ǿ� �ִ��� ���� �ִ��� Ȯ�� �� ��. �� ���� ������ ���׸����̼� ��Ʈ �߻�.

//1) 2022-7�� 8�� ����, �Ѿ�� ���̴н� ��ǻ�Ϳ����� ����� �� ���ư��淡, �˾ƺ��� ���� movementInfo.in�� /spider-man-no-way-home-1.mp4	0	datanode1	DISK	datanode1	DISK	1 ������. 
//�׷��Ƿ� ERSBlockMovement������ �������� �켱 ���� ���� � �������� �� ��, ������ ���� �ɷ��� ���� movementInfo ���� ���� �����ϰ�, ���̴н� ��ǻ�Ϳ� �ݿ��� ���� ���·� ����
//���⼭ �̴°� movementInfo.in�� /spider-man-no-way-home-1.mp4	datanode1	qlc03 datanode1	qlc01 ������.
//2) ���̱׷��̼� �� SSDListParser.sh, fileListParser.sh, �� �˰���, hdfs ERSBlockMovement ��, hdfs ERSBlockPlacement ���� ������� ����Ǵ� ���� ¥�߰���. 
//�̰� ERSBlockMovement �ȿ� �� �� �ִ� ������, ���ο� ������ ���ε� �ɶ��� ERSBlockPlacement�� ����ϴµ� ERSBlockMovement �ȿ��� ERSBlockPlacement ȣ���ϱ�� ��..
//�׷� ������ �� ���α׷����� ���� ������ ����.
//3) ERSBlockPlacement -> FilePlacement, ERSBlockMovement -> FileMovement �� Ŀ�ǵ� ��ü�ϱ�

#include "header.h"
#define NUM_OF_DATEs 3 // for simulation 1 5 15 30
#define NUM_OF_TIMEs 3

int placement_method = 1; // 2~6���� �ٲٸ� �񱳽�Ŵ
int migration_method = 7; // 8~11�� �ٲٸ� �񱳽�Ŵ

int num_of_SSDs = 20; 
int num_of_videos = 1000;
int num_of_new_videos = 78;

double num_of_request_per_sec = 5000; //1000���϶� �ӽð�
//double num_of_request_per_sec = 1000; �� �ڵ� ���ư����� ���� 10�� �ִ� �����ϰ� ������ Ȯ���� ��... 500~1000���� �����

int main(int argc, char* argv[]) {
	//argv �Ķ���Ͱ� ������ �׽�Ʈ ���, ������ �ùķ��̼� ������ ���α׷��� ¥��.
	switch (argc)
	{
	case 1:
		//migartion_in_simulation();
		//placement(true);
		//placement(false);
		migration_in_testbed(1);
	break;
	case 2:
		if (!strcmp(argv[1], "placement")) {
			placement(false);
		}
		else
			printf("command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	case 3:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			migration_in_testbed(stoi(argv[2])); 
			// 1, 2, 3, 4 �̷��� �� ���� ���° migration������ �޵��� ��.
			//���߿� Ȯ�� �� �ð� �޾Ƽ� �Ľ��Ұ� 
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
			placement(true);
		}
		else
			printf("command�� �ùٸ��� �ʽ��ϴ�. �ٽ� ������ �ּ���.\n");
		break;
	case 7:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			num_of_SSDs = stoi(argv[2]);
			num_of_videos = stoi(argv[3]);
			num_of_new_videos = stoi(argv[4]); // NUM_OF_TIMEs;
			num_of_request_per_sec = stod(argv[5]);
			migration_method = stod(argv[6]);
			migartion_in_simulation();
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

void placement(bool _is_simulation) {
	SSD* SSD_list;
	VIDEO_CHUNK* VIDEO_CHUNK_list;

	if (_is_simulation) {
		SSD_list = new SSD[num_of_SSDs + 1];
		VIDEO_CHUNK_list = new VIDEO_CHUNK[num_of_videos];
		setting_for_placement_in_simulation(SSD_list, VIDEO_CHUNK_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);
	}
	else {
		int dummy = 0;
		SSD_list = SSD_initalization_for_testbed(num_of_SSDs);
		VIDEO_CHUNK_list = video_initalization_for_testbed(dummy, num_of_videos, num_of_SSDs, num_of_request_per_sec, -987654321, false); // dummy�� -987654321 �־��ذ� ���� ����... �� ���ϱ�
		setting_for_placement_in_testbed(SSD_list, VIDEO_CHUNK_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);
	}

	int placement_num = placement(SSD_list, VIDEO_CHUNK_list, placement_method, num_of_SSDs, num_of_videos);
	double sum_for_AVG_in_placement = 0;
	double sum_for_STD_in_placement = 0;

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
	if(_is_simulation)
		printf("%lf\n", total_bandwidth_usage_in_placement);
	else {
		for (int vid = 0; vid < num_of_videos + num_of_new_videos; vid++) {
			if (VIDEO_CHUNK_list[vid].assigned_SSD <= 0) {
				VIDEO_CHUNK_list[vid].assigned_SSD = 21 + rand() % 4;
			}
		}
		create_placement_infomation(SSD_list, VIDEO_CHUNK_list, num_of_SSDs, num_of_videos);
		create_result(SSD_list, VIDEO_CHUNK_list, num_of_SSDs, num_of_videos, false);
	}
	
	delete[](SSD_list);
	delete[](VIDEO_CHUNK_list);
}

void migartion_in_simulation() {
	placement_method = PLACEMENT_RANDOM;
	SSD* SSD_list = new SSD[num_of_SSDs + 1];
	VIDEO_CHUNK* VIDEO_CHUNK_list = new VIDEO_CHUNK[num_of_videos];
	setting_for_placement_in_simulation(SSD_list, VIDEO_CHUNK_list, num_of_SSDs, num_of_videos, num_of_request_per_sec);
	placement(SSD_list, VIDEO_CHUNK_list, placement_method, num_of_SSDs, num_of_videos);
	//�������� ������ placement �� ������ migaration�� ����ȴ�.

	int total_migration_num = 0;
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			//�Ʒ��� ���ο� ���� �߰� ����
			if (num_of_new_videos > 0) {
				// ���ο� ���� �߰��� ���� ���� �������� ������Ʈ ����.
				VIDEO_CHUNK* new_VIDEO_CHUNK_list = new VIDEO_CHUNK[num_of_new_videos];
				setting_for_migration_in_simulation(SSD_list, VIDEO_CHUNK_list, new_VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, num_of_request_per_sec, time);

				VIDEO_CHUNK* _VIDEO_CHUNK_conbined_list = new VIDEO_CHUNK[num_of_videos + num_of_new_videos];
				copy(VIDEO_CHUNK_list, VIDEO_CHUNK_list + num_of_videos, _VIDEO_CHUNK_conbined_list);
				delete[] VIDEO_CHUNK_list;
				copy(new_VIDEO_CHUNK_list, new_VIDEO_CHUNK_list + num_of_new_videos, _VIDEO_CHUNK_conbined_list + num_of_videos);
				delete[] new_VIDEO_CHUNK_list;
				VIDEO_CHUNK_list = _VIDEO_CHUNK_conbined_list;
				num_of_videos += num_of_new_videos;  //���� ���� ����Ʈ�� ���ο� ���� �߰�
			}
			else {
				//���ο� ���� ������Ʈ ���ϰ�, �α⵵�� �ٲ� �� ��. 
				setting_for_migration_in_simulation(SSD_list, VIDEO_CHUNK_list, NULL, migration_method, num_of_SSDs, num_of_videos, 0, num_of_request_per_sec, time);
			}
			//migration ����
			//printf("%d��-%d ", day, time);
			if(migration_method >= MIGRATION_OURS)
				total_migration_num += migration(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos);
			else 
				total_migration_num += placement(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos);
			//printf("migration_num %d\n", migration_num);
		}

		// ��� ��� : SSD�� ���, ǥ������ ADWD ���
		if (day == NUM_OF_DATEs) {
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
			printf("%lf\t%lf\t%lf\t%d\n", total_bandwidth_usage_in_migration, sum_for_AVG_in_migration / num_of_SSDs, sqrt(sum_for_STD_in_migration / num_of_SSDs), total_migration_num / (NUM_OF_DATEs * NUM_OF_TIMEs));
			//printf("day%d\n", day);
		}
	}
/*
	double total_bandwidth_of_alloc_videos = 0;
	int num_of_alloc_videos = 0;
	for (int vid = 0; vid < num_of_videos; vid++) {
		if ( !(VIDEO_CHUNK_list[vid].assigned_SSD == NONE_ALLOC && VIDEO_CHUNK_list[vid].assigned_SSD == NONE_ALLOC)) {
			num_of_alloc_videos++;
			total_bandwidth_of_alloc_videos += VIDEO_CHUNK_list[vid].requested_bandwidth;
		}
		if (VIDEO_CHUNK_list[vid].assigned_SSD == VIRTUAL_SSD) {
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
	delete[](VIDEO_CHUNK_list);
}

void migration_in_testbed(int _time) {
	SSD* SSD_list = SSD_initalization_for_testbed(num_of_SSDs);
	VIDEO_CHUNK* VIDEO_CHUNK_list = video_initalization_for_testbed(num_of_videos, num_of_new_videos, num_of_SSDs, num_of_request_per_sec, migration_method, true);

	setting_for_migration_in_testbed(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, num_of_request_per_sec, _time);
	//������ ����Ǿ��ִ� ������, ���ο� ������ ������ �о��

	int* prev_assigned_SSD = new int[num_of_videos];
	for (int vid = 0; vid < num_of_videos; vid++) {
		prev_assigned_SSD[vid] = VIDEO_CHUNK_list[vid].assigned_SSD;
		
		//�����ͳ��6�� HDD ������ ����� ���̱� ������, �˰��򿡼��� HDD ���� �ϳ��� ���� ���Ƿ�
		if (VIDEO_CHUNK_list[vid].assigned_SSD > num_of_SSDs){
			if (migration_method >= MIGRATION_OURS) {
				VIDEO_CHUNK_list[vid].assigned_SSD = VIRTUAL_SSD;
			}
			else {
				VIDEO_CHUNK_list[vid].assigned_SSD = NONE_ALLOC;
			}
		}
	}
	
	//���� migration
	int migration_num;
	if (migration_method >= MIGRATION_OURS)
		migration_num = migration(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos + num_of_new_videos);
	else
		migration_num = placement(SSD_list, VIDEO_CHUNK_list, placement_method, num_of_SSDs, num_of_videos + num_of_new_videos);

	//datanode 6�� HDD ������ ����ϱ� ����. � ���丮���� ������ ���� ���� 
	//(�߿��Ѱ� �ƴ϶� �̷��� ������ �����ߴµ�, �����δ� HDD�� ���丮�� �뷮�� �󸶳� ���Ҵ��� ����ؼ� �־����)
	for (int vid = 0; vid < num_of_videos + num_of_new_videos; vid++) {
		if (VIDEO_CHUNK_list[vid].assigned_SSD <= 0) {
			VIDEO_CHUNK_list[vid].assigned_SSD = 21 + rand() % 4;
		}
	}

	create_migration_infomation(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, prev_assigned_SSD); // �̵� ���� ���� ����
	create_result(SSD_list, VIDEO_CHUNK_list, num_of_SSDs, num_of_videos + num_of_new_videos, true);
	delete[] VIDEO_CHUNK_list;
}

