#include "head.h"

//알고리즘 여기서부터 이제 짜야함
short selected_set[CHANNEL_NUM + 1]; // 각 채널에서 사용하는 비트레이트 set
//short selected_ES[CHANNEL_NUM + 1];// 각 채널이 어떤 es에서 할당되었는가.
//오리지널 버전은 트랜스코딩 안해서 배열 크기가 저렇다.

short** selected_ES;

double used_GHz[ES_NUM + 1];
double total_transfer_data_size[ES_NUM + 1];//실시간으로 전송하는 데이터 사이즈의 합 계산을 위해

short ES_total_count[ES_NUM + 1];

void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	selected_ES = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int row = 1; row <= CHANNEL_NUM; row++) {
		selected_ES[row] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // 오리지널 버전은 트랜스코딩 안하니까
			selected_ES[row][col] = -1;
		}
	}

	memset(ES_total_count, 0, (sizeof(short) * (ES_NUM + 1)));
	double first_GHz = 0; //lowest version만 트랜스코딩할때

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
	}
	for (int ES = 0; ES <= ES_NUM; ES++) {
		used_GHz[ES] = 0;
		total_transfer_data_size[ES] = 0;
	}

	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}

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
	set<pair<double, pair<int, int>>> list_VSD;
	//_version_set->version_set_num(N^set)으로 초기화한 상태에서 set을 내림.
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
	set<pair<double, int>> remained_GHz_of_ESs_set;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		remained_GHz_of_ESs_set.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		set <pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.end();

		int ES = -1;
		double GHz = 0;

		bool is_allocated_ingestion_server = false;
		while (true) {
			pos--;
			if (pos == remained_GHz_of_ESs_set.begin()) {
				is_allocated_ingestion_server = true;
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[1] >= 0)) {
				break;
			}
		}

		if (!is_allocated_ingestion_server) {
			selected_ES[ch][1] = ES;
			ES_total_count[ES]++;

			used_GHz[ES] += _channel_list[ch].video_GHz[1];
			total_transfer_data_size[ES] += _version_set->data_size[1];

			remained_GHz_of_ESs_set.erase(pos);
			remained_GHz_of_ESs_set.insert(make_pair(GHz - _channel_list[ch].video_GHz[1], ES));
		}
		else {
			if (used_GHz[0] + _channel_list[ch].video_GHz[1] <= _server_list[0].processing_capacity) {
				selected_ES[ch][1] = 0;
				ES_total_count[0]++;
				used_GHz[0] += _channel_list[ch].video_GHz[1];
				total_transfer_data_size[0] += _version_set->data_size[1];
			}
		}
	}

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > list_CA_initialization;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = selected_set[ch];
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // 이전에 선택한 set에서 할당했던 GHz는 전부 삭제해 준다. 
				double slope = _channel_list[ch].pwq[ver] / _channel_list[ch].video_GHz[ver];
				list_CA_initialization.insert(make_pair(slope, make_pair(ch, ver)));
			}
		}
	}
	while (list_CA_initialization.size()) {
		int ch = (*list_CA_initialization.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*list_CA_initialization.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		list_CA_initialization.erase(list_CA_initialization.begin());//맨 앞 삭제함

		set <pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.end();
		int ES = -1;
		double GHz = 0;
		bool is_allocated_ingestion_server = false;

		while (true){
			pos--;
			if (pos == remained_GHz_of_ESs_set.begin()) {
				is_allocated_ingestion_server = true;
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[ver] >= 0)) {
				break;
			}
		}

		if (!is_allocated_ingestion_server) {
			selected_ES[ch][ver] = ES;
			ES_total_count[ES]++;

			used_GHz[ES] += _channel_list[ch].video_GHz[ver];
			total_transfer_data_size[ES] += _version_set->data_size[ver];

			remained_GHz_of_ESs_set.erase(pos);
			remained_GHz_of_ESs_set.insert(make_pair(GHz - _channel_list[ch].video_GHz[ver], ES));
		}
		else {
			if (used_GHz[0] + _channel_list[ch].video_GHz[ver] <= _server_list[0].processing_capacity) {
				selected_ES[ch][ver] = 0;
				ES_total_count[0]++;
				used_GHz[0] += _channel_list[ch].video_GHz[ver];
				total_transfer_data_size[0] += _version_set->data_size[ver];
			}
		}
	}

	//set 계산하기
	set_version_set(_version_set, selected_set, selected_ES);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	double total_cost = 0;
	double remained_GHz[ES_NUM + 1]; // processing capacity[es] - used_GHz[es] 하면 remained_GHz[es] 하면 나옴. 모든 노드의 남은 GHz 계산을 위해.
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if (ES_total_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
			remained_GHz[ES] = _server_list[ES].processing_capacity - used_GHz[ES];
		}
	}
	std::printf("=CA-init= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);


	// 2-2. CA-redistribution phase
	// 아님. 완전 엎어야함. migration이 아니고 ES에 할당된 version 중에서 빼야함.
	// 이 때 ES에서 뺄 때, ingesion server에 있는 버전보다 pwq가 높을 경우, 
	// (즉 ingestion server에 할당된 버전 중, pwq가 제일 낮은 버전과 비교한다.)
	// (ingestion server에 할당된 버전이 빼려는 버전보다 더 pwq가 낮을 경우, 해당 버전은 ingestion server에 들어가고 원래 거기 있던 버전은 빠짐.)
	// ES에서 뺀 것은 다시 ingestion server에 보내고, ingesion server에서 비교 버전을 완전히 뺀다.
	set<pair<double, pair<int, int>>> list_CA_redistribution;
	// slope (pwq/cost) 값 / channel-version
	
	set<pair<double, pair<int, int>>> pwq_of_version_in_ingestion_server;
	// pwq 값 / channel-version
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		double cost = 0;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (selected_ES[ch][ver] > 0) { //-1은 할당 안 됨, 0은 ingestion server
				//double slope = _channel_list[ch].pwq[ver] / calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);
				double reduced_cost = (calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024)
					- calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), (total_transfer_data_size[selected_ES[ch][ver]] - _version_set->data_size[ver]) / 1024));
				double slope = _channel_list[ch].pwq[ver] / reduced_cost;

				list_CA_redistribution.insert(make_pair(slope, make_pair(ch, ver)));
			}

			else if (selected_ES[ch][ver] == 0) { // 0은 ingestion server에 할당된 값
				pwq_of_version_in_ingestion_server.insert(make_pair(_channel_list[ch].pwq[ver], make_pair(ch, ver)));
			}
		}
	}


	while (list_CA_redistribution.size()) {
		int ch = (*list_CA_redistribution.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*list_CA_redistribution.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		list_CA_redistribution.erase(list_CA_redistribution.begin());//맨 앞 삭제함

		/*if (used_GHz[selected_ES[ch][ver]] < 0) {
			cout << "error";
		}*/

		double prev_cost = calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);
		total_transfer_data_size[selected_ES[ch][ver]] -= _version_set->data_size[ver];
		double curr_cost = calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);


		ES_total_count[selected_ES[ch][ver]]--;

		if (!ES_total_count[selected_ES[ch][ver]]) {
			used_GHz[selected_ES[ch][ver]] = 0;
			total_cost -= prev_cost;
		}
		else {
			used_GHz[selected_ES[ch][ver]] -= _channel_list[ch].video_GHz[ver];
			total_cost -= (prev_cost - curr_cost);
		}

		//여기까지는 cost 때문에 ES에서 version 빼는 것.
		//이제 이 뺀 version을 ingestion server에 할당 할 수 있을지를 봐야한다.
		//정확히는, ingestion server의 가장 낮은 pwq를 가진 version과 pwq를 비교한다.

		int ch_in_ingestion_server = (*pwq_of_version_in_ingestion_server.begin()).second.first;
		int ver_in_ingestion_server = (*pwq_of_version_in_ingestion_server.begin()).second.second;
		double pwq_in_ingestion_server = (*pwq_of_version_in_ingestion_server.begin()).first;
		double video_GHz_in_ingestion_server = _channel_list[ch_in_ingestion_server].video_GHz[ver_in_ingestion_server];
		if ((pwq_in_ingestion_server < _channel_list[ch].pwq[ver]) &&
			((used_GHz[0] - video_GHz_in_ingestion_server + _channel_list[ch].video_GHz[ver]) <= _server_list[0].processing_capacity)) {
			used_GHz[0] -= video_GHz_in_ingestion_server;
			used_GHz[0] += _channel_list[ch].video_GHz[ver];

			pwq_of_version_in_ingestion_server.erase(pwq_of_version_in_ingestion_server.begin());
			pwq_of_version_in_ingestion_server.insert(make_pair(_channel_list[ch].pwq[ver], make_pair(ch, ver)));
			//여기까지
			selected_ES[ch][ver] = 0;
		}
		else {
			selected_ES[ch][ver] = -1;
		}

		if (total_cost <= cost_limit) {
			break;
		}
	}

	//set 계산하기
	set_version_set(_version_set, selected_set, selected_ES);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if (ES_total_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
			remained_GHz[ES] = _server_list[ES].processing_capacity - used_GHz[ES];
		}
	}
	std::printf("=최종= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);
}

void set_version_set(bitrate_version_set* _version_set, short** _selected_ES) {
	//set 계산하기
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = 1;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (selected_ES[ch][ver] != -1)
				set += _version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1));
			//다 계산하고 +1할것
		}
		selected_set[ch] = set;
	}
}