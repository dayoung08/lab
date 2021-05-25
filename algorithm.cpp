#include "head.h"

//알고리즘 여기서부터 이제 짜야함
short selected_set[CHANNEL_NUM + 1]; // 각 채널에서 사용하는 비트레이트 set
short selected_ES[CHANNEL_NUM + 1];// 각 채널이 어떤 es에서 할당되었는가.
//오리지널 버전은 트랜스코딩 안해서 배열 크기가 저렇다.

double remained_GHz[ES_NUM + 1];// processing capacity[es] - remained_GHz[es] 하면 used_GHz[es] 나옴. 모든 노드의 남은 GHz 계산을 위해.
double total_transfer_data_size[ES_NUM + 1];//실시간으로 전송하는 데이터 사이즈의 합 계산을 위해

short ES_count[ES_NUM + 1];

void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	memset(ES_count, 0, (sizeof(short) * (ES_NUM + 1)));
	double first_GHz_temp = 0; //lowest version만 트랜스코딩할때
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		first_GHz_temp += _channel_list[ch].sum_of_version_set_GHz[1];
	}

	double GHz_limit = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		remained_GHz[ES] = 0;
		GHz_limit += _server_list[ES].processing_capacity;
		total_transfer_data_size[ES] = 0;
	}

	if (GHz_limit < first_GHz_temp) {
		printf("GHz가 모자란 상황/Channel 수를 줄이거나, 엣지 수를 늘릴 것\n");
		printf("lowest version만 트랜스코딩 했을 때 %lf GHz / %lf GHz\n\n", first_GHz_temp, GHz_limit);
		exit(0);
	}


	//나중에 각 페이즈마다 함수 생성할 것. 그래야 보는 게 편하다.
	//1. VSD phase
	double total_GHz = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//selected_ES[ch] = 0;
		selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}
	set<pair<double, pair<int, int>>, less<pair<double, pair<int, int>>> > list_VSD;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int set = 1; set <= _version_set->version_set_num - 1; set++) { //소스는 전부 1080p
			double slope = (_channel_list[ch].sum_of_pwq[_version_set->version_set_num] - _channel_list[ch].sum_of_pwq[set]) / (_channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num] - _channel_list[ch].sum_of_version_set_GHz[set]);
			list_VSD.insert(make_pair(slope, make_pair(ch, set)));
		}
	}

	while (list_VSD.size()) {
		int ch = (*list_VSD.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int set = (*list_VSD.begin()).second.second; //slope가 가장 큰 것은 어떤 세트인가?

		list_VSD.erase(list_VSD.begin());//맨 앞 삭제함
		//int prev_엣지_node = selected_BN[channel];
		int prev_set = selected_set[ch];
		if (_channel_list[ch].sum_of_version_set_GHz[set] < _channel_list[ch].sum_of_version_set_GHz[prev_set]) {
			double expected_total_GHz = total_GHz - _channel_list[ch].sum_of_version_set_GHz[prev_set] + _channel_list[ch].sum_of_version_set_GHz[set];
			total_GHz = expected_total_GHz;
			selected_set[ch] = set;
			if (expected_total_GHz < GHz_limit) {
				break;
			}
		}
	}

	double total_GHz_temp = 0;
	double total_pwq_temp = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz_temp += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq_temp += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	std::printf("=중간과정= total_GHz_temp : %lf GHz, total_pwq_temp : %lf\n", total_GHz_temp, total_pwq_temp);
	std::printf("=모든 엣지 서버의 총 합 GHz : %lf GHz\n\n", GHz_limit);

	// 2-1. CA-initialization phase
	set<pair<double, int>, greater<pair<double, int>> > list_CA_initialization;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		double slope = _channel_list[ch].sum_of_pwq[selected_set[ch]] / _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		list_CA_initialization.insert(make_pair(slope, ch));
	}

	set<pair<double, int>, greater<pair<double, int>>> highest_GHz_first; // set을 쓰면 자동 정렬이 되어 가장 남은 GHz가 많은 엣지 서버가 맨 위로 감.
	double highest_GHz_first_array[ES_NUM + 1]; // set으로는 GHz 변경에 따른 update가 좀 복잡해서 따로 array도 선언해서 update를 도움.

	//각 페이즈마다 함수 생성할 것. 그래야 보는 게 편하다.
	for (int ES = 1; ES <= ES_NUM; ES++) {
		highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
		highest_GHz_first_array[ES] = _server_list[ES].processing_capacity; //array
	}

	while (list_CA_initialization.size()) {
		int ch = (*list_CA_initialization.begin()).second; // slope가 가장 큰 것은 어떤 채널인가?
		list_CA_initialization.erase(list_CA_initialization.begin());//맨 앞 삭제함

		int queue_cnt = 1;
		int confirm_cnt = 1;
		short unavailable_ES_queue[ES_NUM+1];
		memset(unavailable_ES_queue, 0, (sizeof(short) * (ES_NUM + 1)));

		while (!highest_GHz_first.empty()) {
			int es = (*highest_GHz_first.begin()).second;
			if (!_channel_list[ch].available_server_list[es]) {
				unavailable_ES_queue[queue_cnt] = es;
				queue_cnt++;
				highest_GHz_first.erase(*highest_GHz_first.begin());
			}
			confirm_cnt++;
			if (confirm_cnt > ES_NUM){
				break;
			}
		}

		//커버리지 제약을 만족하는 ES를 선택하기 위함.
		//만약 highest_remained_utility_first가 비었다면 이건 그냥 ingestion server로 가야함.
		if (!highest_GHz_first.empty()) { // 선택된 노드가 아직 꽉 차지 않았다면
			int ES = (*highest_GHz_first.begin()).second;
			if (remained_GHz[ES] + _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]] <= _server_list[ES].processing_capacity) {// ingestion server
				double slope = _channel_list[ch].sum_of_pwq[selected_set[ch]] / _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
				selected_ES[ch] = ES;
				remained_GHz[ES] += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
				
				highest_GHz_first.erase(*highest_GHz_first.begin());
				highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity - remained_GHz[ES], ES)); //선택된 엣지에 할당
				highest_GHz_first_array[ES] = _server_list[ES].processing_capacity - remained_GHz[ES]; //array 갱신

				total_transfer_data_size[ES] += _channel_list[ch].sum_of_transfer_data_size[selected_set[ch]];

				ES_count[ES]++;
			}
			else { // ingestion server
				selected_ES[ch] = 0;
				ES_count[0]++;
			}
		}
		else { // ingestion server
			selected_ES[ch] = 0;
			ES_count[0]++;
		}

		//highest_remained_utility_first에 다시 커버리지 안 맞았던 ES들 삽입.
		for (int cnt = 1; cnt < queue_cnt; cnt++) {
			highest_GHz_first.insert(make_pair(highest_GHz_first_array[unavailable_ES_queue[cnt]], unavailable_ES_queue[cnt]));
		}
	}


	// 2-2. CA-migration phase
	double total_cost = 0;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		if (ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
		}
	}
	//remained_GHz 확인해보기

	set<pair<double, int>, greater<pair<double, int>> > list_CA_migration;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		if (selected_ES[ch] != 0) {
			double slope = _channel_list[ch].sum_of_pwq[selected_set[ch]] / calculate_ES_cost(&(_server_list[selected_ES[ch]]), total_transfer_data_size[selected_ES[ch]] / 1024);
			list_CA_migration.insert(make_pair(slope, ch));
		}
	}

	while (total_cost > cost_limit) {
		if (!list_CA_migration.size()) {
			break;
		}

		int ch = (*list_CA_migration.begin()).second; // slope가 가장 큰 것은 어떤 채널인가?
		list_CA_migration.erase(list_CA_migration.begin());//맨 앞 삭제함


		double prev_cost =  calculate_ES_cost(&(_server_list[selected_ES[ch]]), total_transfer_data_size[selected_ES[ch]] / 1024);
		total_transfer_data_size[selected_ES[ch]] -= _channel_list[ch].sum_of_transfer_data_size[selected_set[ch]];
		double curr_cost = calculate_ES_cost(&(_server_list[selected_ES[ch]]), total_transfer_data_size[selected_ES[ch]] / 1024);
		

		ES_count[selected_ES[ch]]--;
		ES_count[0]++;
		selected_ES[ch] = 0;

		remained_GHz[selected_ES[ch]] += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		remained_GHz[0] -= _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		
		total_cost -= (prev_cost - curr_cost);
	}

	//finalization은 알고리즘 상에서 temp값 -> 진짜 값 확정하는 과정이라 여기선 필요 x
	//but 아래는 각 노드 마다 할당된 채널들을 정리하려고 하는 과정.
	double total_pwq = 0;
	double total_video_quality = 0;
	double used_GHz[ES_NUM + 1];

	/*int* number_of_transcoding = (int*)malloc(sizeof(int) * (_version_set->version_num));
	for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
		number_of_transcoding[ver] = 0;
	}*/
	// 각 비트레이트 버전 중 실제 transcoding된 갯수 계산 용. 이 갯수의 range는 [0, CHANNEL_NUM]

	for (int ES = 0; ES <= ES_NUM; ES++) {
		used_GHz[ES] = 0;
	}
	std::printf("<<VDA-Greedy>>\n");
	//printf("[채널-세트]\n");

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
		total_video_quality += _channel_list[ch].sum_of_video_quality[selected_set[ch]];
		used_GHz[selected_ES[ch]] += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
	}

	std::printf("\n전체 pwq의 합 : %lf", total_pwq);
	// 이게 차이가 중간과정에서 나온 pwq와 차이가 없게 출력될 때 그 이유는, 엣지 할당을 못한 버전은 매우 작은 pwq를 가졌기 때문이다. 즉 차이는 있는데 소숫점 짤렸다. 
	std::printf("\n전체 video_quality의 평균 : %lf\n\n", total_video_quality / CHANNEL_NUM / _version_set->version_num);
	/*for (int per_index = 0; per_index < 10; per_index++) {
		printf("%d%~%d%% pwq의 합/video_quality의 평균 : %lf, %lf\n", (per_index * 10), (per_index + 1) * 10, pwq_sum_range[per_index], video_quality_sum_range[per_index] / (CHANNEL_NUM / 10) / info->version_num);
	}*/

	double sum_cost_final = 0;
	double sum_GHz_final = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if(ES > 0 && ES_count[ES] > 0)
			sum_cost_final += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
		sum_GHz_final += used_GHz[ES];
	}

	std::printf("\n사용 비용 : %lf $\n", sum_cost_final);
	std::printf("사용 GHz : %lf GHz\n\n", sum_GHz_final);

	/*for (int version = _version_set->version_num - 1; version >= 2; version--) {
		printf("버전 %d : %d\n", version, number_of_transcoding[version]);
	}
	printf("\n");*/
}