#include "head.h"

//알고리즘 여기서부터 이제 짜야함
short selected_set[CHANNEL_NUM + 1]; // 각 채널에서 사용하는 비트레이트 set
//short selected_ES[CHANNEL_NUM + 1];// 각 채널이 어떤 es에서 할당되었는가.
//오리지널 버전은 트랜스코딩 안해서 배열 크기가 저렇다.

short** selected_ES;

double remained_GHz[ES_NUM + 1];// processing capacity[es] - remained_GHz[es] 하면 used_GHz[es] 나옴. 모든 노드의 남은 GHz 계산을 위해.
double used_GHz[ES_NUM + 1];
double total_transfer_data_size[ES_NUM + 1];//실시간으로 전송하는 데이터 사이즈의 합 계산을 위해

short ES_count[ES_NUM + 1];

void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	selected_ES = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int row = 1; row <= CHANNEL_NUM; row++) {
		selected_ES[row] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int col = 1; col <= _version_set->version_num-1; col++) {  // 오리지널 버전은 트랜스코딩 안하니까
			selected_ES[row][col] = -1;
		}
	}

	memset(ES_count, 0, (sizeof(short) * (ES_NUM + 1)));
	double first_GHz = 0; //lowest version만 트랜스코딩할때
	
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
	}
	for (int ES = 0; ES <= ES_NUM; ES++) {
		remained_GHz[ES] = 0;
		total_transfer_data_size[ES] = 0;
	}

	//각 ES의 커버리지를 확인하고 전체 채널 중 각 몇개의 채널에서 할당이 가능한지 퍼센테이지를 구하고, 그 걸 processing capacity에 곱해보자.
	double GHz_limit = _server_list[0].processing_capacity;
	//double GHz_limit_2 = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		int alloc_ch_cnt = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			if (_channel_list[ch].available_server_list[ES]) {
				alloc_ch_cnt++;
			}
		}

		//GHz_limit += _server_list[ES].processing_capacity * (((double)alloc_ch_cnt) / CHANNEL_NUM);
		GHz_limit += _server_list[ES].processing_capacity;
	}

	//여기까지 210530 수정. coverage를 따져서 processing capacity를 노멀라이즈 했음.

	printf("lowest version만 트랜스코딩 했을 때 %lf GHz / GHz 총 합 %lf GHz\n\n", first_GHz, GHz_limit);
	if (GHz_limit < first_GHz) {
		printf("GHz가 모자란 상황/Channel 수를 줄이거나, 엣지 수를 늘릴 것\n");
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

	total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}

	std::printf("=VSD= total_GHz : %lf GHz, total_pwq : %lf\n", total_GHz, total_pwq);

	// 2-1. CA-initialization phase
	set<pair<double, int>, greater<pair<double, int>>> highest_GHz_first; // set을 쓰면 자동 정렬이 되어 가장 남은 GHz가 많은 엣지 서버가 맨 위로 감.
	double highest_GHz_first_array[ES_NUM + 1]; // set으로는 GHz 변경에 따른 update가 좀 복잡해서 따로 array도 선언해서 update를 도움.

	//각 페이즈마다 함수 생성할 것. 그래야 보는 게 편하다.
	for (int ES = 1; ES <= ES_NUM; ES++) {
		highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
		highest_GHz_first_array[ES] = _server_list[ES].processing_capacity; //array
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int ES = (*highest_GHz_first.begin()).second; // 가장 남은 GHz가 많은 백엔드는 무엇인가?
		float GHz = (*highest_GHz_first.begin()).first; // 그 백엔드의 GHz는 얼마인가?

		if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
			selected_ES[ch][1] = ES;

			highest_GHz_first.erase(highest_GHz_first.begin());
			highest_GHz_first.insert(make_pair((GHz - _channel_list[ch].video_GHz[1]), ES)); //set 갱신
			highest_GHz_first_array[ES] = (GHz - _channel_list[ch].video_GHz[1]); //map 갱신

			remained_GHz[ES] += _channel_list[ch].video_GHz[1];
		}
		else {
			selected_ES[ch][1] = 0;
			ES_count[0]++;
			remained_GHz[0] += _channel_list[ch].video_GHz[1];

			if (remained_GHz[0] >= _server_list[0].processing_capacity) {
				//printf("문제 발생함, 할당한 채널 개수 %d\n", alloc_num);
			}
		}
	}

	set<pair<long float, pair<int, int>>, greater<pair<long float, pair<int, int>>> > list_CA_initialization;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = selected_set[ch];
		for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // 이전에 선택한 set에서 할당했던 GHz는 전부 삭제해 준다. 
				float slope = (_channel_list[ch].pwq[ver] / (_channel_list[ch].video_GHz[ver]));
				list_CA_initialization.insert(make_pair(slope, make_pair(ch, ver)));
				//cnt++;
			}
		}
	}
	int last_version_num = 0;
	while (list_CA_initialization.size()) {
		int ch = (*list_CA_initialization.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*list_CA_initialization.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		list_CA_initialization.erase(list_CA_initialization.begin());//맨 앞 삭제함

		int queue_cnt = 1;
		int confirm_cnt = 1;
		short unavailable_ES_queue[ES_NUM + 1];
		memset(unavailable_ES_queue, 0, (sizeof(short) * (ES_NUM + 1)));

		//alloc_num++;
		while (!highest_GHz_first.empty()) {
			int es = (*highest_GHz_first.begin()).second;
			if (!_channel_list[ch].available_server_list[es]) {
				unavailable_ES_queue[queue_cnt] = es;
				queue_cnt++;
				highest_GHz_first.erase(*highest_GHz_first.begin());
			}
			confirm_cnt++;
			if (confirm_cnt > ES_NUM) {
				break;
			}
		}

		//커버리지 제약을 만족하는 ES를 선택하기 위함.
		//만약 highest_remained_utility_first가 비었다면 이건 그냥 ingestion server로 가야함.
		if (!highest_GHz_first.empty()) { // 선택된 노드가 아직 꽉 차지 않았다면
			int ES = (*highest_GHz_first.begin()).second;
			if (remained_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity) {// ingestion server
				selected_ES[ch][ver] = ES;
				remained_GHz[ES] += _channel_list[ch].video_GHz[ver];

				highest_GHz_first.erase(*highest_GHz_first.begin());
				highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity - remained_GHz[ES], ES)); //선택된 엣지에 할당
				highest_GHz_first_array[ES] = _server_list[ES].processing_capacity - remained_GHz[ES]; //array 갱신

				total_transfer_data_size[ES] += _version_set->data_size[ver];
				ES_count[ES]++;
			}
			else { // ingestion server
				if (remained_GHz[0] < _server_list[0].processing_capacity) {
					selected_ES[ch][ver] = 0;
					ES_count[0]++;
					remained_GHz[0] += _channel_list[ch].video_GHz[ver];
				}
			}
		}
		else { // ingestion server
			if (remained_GHz[0] < _server_list[0].processing_capacity) {
				selected_ES[ch][ver] = 0;
				ES_count[0]++;
				remained_GHz[0] += _channel_list[ch].video_GHz[ver];
			}
		}

		//highest_remained_utility_first에 다시 커버리지 안 맞았던 ES들 삽입.
		for (int cnt = 1; cnt < queue_cnt; cnt++) {
			highest_GHz_first.insert(make_pair(highest_GHz_first_array[unavailable_ES_queue[cnt]], unavailable_ES_queue[cnt]));
		}
	}

	//set 계산하기
	recalculate_set(_version_set);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	double total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if (ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
			used_GHz[ES] = _server_list[ES].processing_capacity - remained_GHz[ES];
		}
	}
	std::printf("=CA-init= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);


	// 2-2. CA-migration phase
	// 아님. 완전 엎어야함. 
	// migration이 아니고 할당된 version 중에서 빼야함.
	set<pair<long float, pair<int, int>>, less<pair<long float, pair<int, int>>> > list_CA_exception;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		double cost = 0;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (selected_ES[ch][ver] > 0) { //-1은 할당 안 됨, 0은 ingestion server
				double slope = _channel_list[ch].pwq[ver] / calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);
				list_CA_exception.insert(make_pair(slope, make_pair(ch, ver)));
			}
		}
	}

	while (list_CA_exception.size()) {
		if (total_cost < cost_limit) {
			break;
		}

		int ch = (*list_CA_exception.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*list_CA_exception.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		list_CA_exception.erase(list_CA_exception.begin());//맨 앞 삭제함

		if (remained_GHz[selected_ES[ch][ver]] < 0) {
			cout << "error";
		}

		if (selected_ES[ch][ver] > 0) {
			double prev_cost = calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);
			total_transfer_data_size[selected_ES[ch][ver]] -= _version_set->data_size[ver];
			double curr_cost = calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);


			ES_count[selected_ES[ch][ver]]--;
			remained_GHz[0] += _channel_list[ch].video_GHz[ver];

			if (!ES_count[selected_ES[ch][ver]]) {
				remained_GHz[selected_ES[ch][ver]] = 0;
				total_cost -= prev_cost;
			}
			else {
				remained_GHz[selected_ES[ch][ver]] -= _channel_list[ch].video_GHz[ver];
				total_cost -= (prev_cost - curr_cost);
			}

			selected_ES[ch][ver] = -1;
		}
	}

	//set 계산하기
	recalculate_set(_version_set);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if (ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
			used_GHz[ES] = _server_list[ES].processing_capacity - remained_GHz[ES];
		}
	}
	std::printf("=최종= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);
}

void recalculate_set(bitrate_version_set* _version_set) {
	//set 계산하기
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = 0;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (selected_ES[ch][ver] != -1)
				set += _version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1));
			//다 계산하고 +1할것
		}
		set += 1;
		selected_set[ch] = set;
	}
}