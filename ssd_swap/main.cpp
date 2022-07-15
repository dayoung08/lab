//testbed에서 placement때는 new_video_list.in만 있어야함. existing_video_list.in이 제대로 된 파일이 아닌 상태로 있으면 infomation_file_create.cpp에서 오류가 발생함.
//또한 testbed에서 migration 때, existing_video_list.in에 datanode와 어떤 SSD에 저장되어 있는지 적혀 있는지 확인 할 것. 안 적혀 있으면 세그멘테이션 폴트 발생.

//1) 2022-7월 8일 기준, 한양대 하이닉스 컴퓨터에서는 제대로 안 돌아가길래, 알아보니 아직 movementInfo.in가 /spider-man-no-way-home-1.mp4	0	datanode1	DISK	datanode1	DISK	1 상태임. 
//그러므로 ERSBlockMovement했을때 오류나면 우선 저거 먼저 어떤 버전인지 볼 것, 컴파일 오래 걸려서 저거 movementInfo 파일 포맷 수정하고도, 하이닉스 컴퓨터엔 반영을 안한 상태로 보임
//여기서 뽑는건 movementInfo.in가 /spider-man-no-way-home-1.mp4	datanode1	qlc03 datanode1	qlc01 포맷임.
//2) 마이그레이션 시 SSDListParser.sh, fileListParser.sh, 이 알고리즘, hdfs ERSBlockMovement 툴, hdfs ERSBlockPlacement 툴이 순서대로 실행되는 셸을 짜야겠음. 
//이걸 ERSBlockMovement 안에 다 안 넣는 이유는, 새로운 파일이 업로드 될때는 ERSBlockPlacement를 써야하는데 ERSBlockMovement 안에서 ERSBlockPlacement 호출하기는 좀..
//그런 이유로 셸 프로그래밍이 제일 무난해 보임.
//3) ERSBlockPlacement -> FilePlacement, ERSBlockMovement -> FileMovement 로 커맨드 교체하기

#include "header.h"
#define NUM_OF_DATEs 3 // for simulation 1 5 15 30
#define NUM_OF_TIMEs 3

int placement_method = 1; // 2~6으로 바꾸면 비교스킴
int migration_method = 7; // 8~11로 바꾸면 비교스킴

int num_of_SSDs = 20; 
int num_of_videos = 1000;
int num_of_new_videos = 78;

double num_of_request_per_sec = 5000; //1000개일때 임시값
//double num_of_request_per_sec = 1000; 이 코드 돌아가는지 파일 10개 있다 가정하고 적당히 확인할 때... 500~1000으로 잡아줌

int main(int argc, char* argv[]) {
	//argv 파라미터가 있으면 테스트 배드, 없으면 시뮬레이션 돌리는 프로그램을 짜자.
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
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	case 3:
		if (!strcmp(argv[1], "movement") || !strcmp(argv[1], "migration")) {
			migration_in_testbed(stoi(argv[2])); 
			// 1, 2, 3, 4 이렇게 각 날의 몇번째 migration인지를 받도록 함.
			//나중에 확장 시 시간 받아서 파싱할것 
		}
		else
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
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
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
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
			printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
		break;
	default:
		printf("command가 올바르지 않습니다. 다시 실행해 주세요.\n");
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
		VIDEO_CHUNK_list = video_initalization_for_testbed(dummy, num_of_videos, num_of_SSDs, num_of_request_per_sec, -987654321, false); // dummy와 -987654321 넣어준건 이유 없음... 안 쓰니까
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
	//랜덤으로 비디오를 placement 한 곳에서 migaration이 수행된다.

	int total_migration_num = 0;
	for (int day = 1; day <= NUM_OF_DATEs; day++) {
		for (int time = 1; time <= NUM_OF_TIMEs; time++) {
			//아래는 새로운 비디오 추가 과정
			if (num_of_new_videos > 0) {
				// 새로운 비디오 추가에 따라 비디오 정보들을 업데이트 해줌.
				VIDEO_CHUNK* new_VIDEO_CHUNK_list = new VIDEO_CHUNK[num_of_new_videos];
				setting_for_migration_in_simulation(SSD_list, VIDEO_CHUNK_list, new_VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, num_of_request_per_sec, time);

				VIDEO_CHUNK* _VIDEO_CHUNK_conbined_list = new VIDEO_CHUNK[num_of_videos + num_of_new_videos];
				copy(VIDEO_CHUNK_list, VIDEO_CHUNK_list + num_of_videos, _VIDEO_CHUNK_conbined_list);
				delete[] VIDEO_CHUNK_list;
				copy(new_VIDEO_CHUNK_list, new_VIDEO_CHUNK_list + num_of_new_videos, _VIDEO_CHUNK_conbined_list + num_of_videos);
				delete[] new_VIDEO_CHUNK_list;
				VIDEO_CHUNK_list = _VIDEO_CHUNK_conbined_list;
				num_of_videos += num_of_new_videos;  //기존 비디오 리스트에 새로운 비디오 추가
			}
			else {
				//새로운 비디오 업데이트 안하고, 인기도만 바꿀 때 씀. 
				setting_for_migration_in_simulation(SSD_list, VIDEO_CHUNK_list, NULL, migration_method, num_of_SSDs, num_of_videos, 0, num_of_request_per_sec, time);
			}
			//migration 수행
			//printf("%d일-%d ", day, time);
			if(migration_method >= MIGRATION_OURS)
				total_migration_num += migration(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos);
			else 
				total_migration_num += placement(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos);
			//printf("migration_num %d\n", migration_num);
		}

		// 결과 출력 : SSD의 평균, 표준편차 ADWD 출력
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
	printf("저장된 비디오 총 갯수 %d/%d\n", num_of_alloc_videos, num_of_videos);
	printf("저장된 비디오의 Total requested bandwidth %lf / %lf\n", total_bandwidth_of_alloc_videos, ((double)VIDEO_BANDWIDTH * (double)num_of_request_per_sec));
	
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
	//기존에 저장되어있던 비디오와, 새로운 비디오의 정보를 읽어옴

	int* prev_assigned_SSD = new int[num_of_videos];
	for (int vid = 0; vid < num_of_videos; vid++) {
		prev_assigned_SSD[vid] = VIDEO_CHUNK_list[vid].assigned_SSD;
		
		//데이터노드6을 HDD 존으로 사용할 것이기 때문에, 알고리즘에서는 HDD 존을 하나로 묶어 쓰므로
		if (VIDEO_CHUNK_list[vid].assigned_SSD > num_of_SSDs){
			if (migration_method >= MIGRATION_OURS) {
				VIDEO_CHUNK_list[vid].assigned_SSD = VIRTUAL_SSD;
			}
			else {
				VIDEO_CHUNK_list[vid].assigned_SSD = NONE_ALLOC;
			}
		}
	}
	
	//비디오 migration
	int migration_num;
	if (migration_method >= MIGRATION_OURS)
		migration_num = migration(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos + num_of_new_videos);
	else
		migration_num = placement(SSD_list, VIDEO_CHUNK_list, placement_method, num_of_SSDs, num_of_videos + num_of_new_videos);

	//datanode 6을 HDD 존으로 사용하기 위함. 어떤 스토리지에 들어갈지는 랜덤 선택 
	//(중요한게 아니라서 이렇게 적당히 구현했는데, 실제로는 HDD의 스토리지 용량이 얼마나 남았는지 계산해서 넣어야함)
	for (int vid = 0; vid < num_of_videos + num_of_new_videos; vid++) {
		if (VIDEO_CHUNK_list[vid].assigned_SSD <= 0) {
			VIDEO_CHUNK_list[vid].assigned_SSD = 21 + rand() % 4;
		}
	}

	create_migration_infomation(SSD_list, VIDEO_CHUNK_list, migration_method, num_of_SSDs, num_of_videos, num_of_new_videos, prev_assigned_SSD); // 이동 정보 파일 생성
	create_result(SSD_list, VIDEO_CHUNK_list, num_of_SSDs, num_of_videos + num_of_new_videos, true);
	delete[] VIDEO_CHUNK_list;
}

