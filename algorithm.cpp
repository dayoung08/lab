#include "head.h"

//알고리즘 여기서부터 이제 짜야함

//double _used_GHz[ES_NUM + 1];
//double total_transfer_data_size[ES_NUM + 1];//실시간으로 전송하는 데이터 사이즈의 합 계산을 위해
//short _ES_count[ES_NUM + 1];

void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit) {
	short selected_set[CHANNEL_NUM + 1]; // 각 채널에서 사용하는 비트레이트 set
	//short selected_ES[CHANNEL_NUM + 1];// 각 채널이 어떤 es에서 할당되었는가.
	//오리지널 버전은 트랜스코딩 안해서 배열 크기가 저렇다.
	short** selected_ES;

	selected_ES = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int row = 1; row <= CHANNEL_NUM; row++) {
		selected_ES[row] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // 오리지널 버전은 트랜스코딩 안하니까
			selected_ES[row][col] = -1;
		}
	}

	double _used_GHz[ES_NUM + 1];
	//double total_transfer_data_size[ES_NUM + 1];//실시간으로 전송하는 데이터 사이즈의 합 계산을 위해
	short _ES_count[ES_NUM + 1];
	memset(_ES_count, 0, (sizeof(short) * (ES_NUM + 1)));
	for (int ES = 0; ES <= ES_NUM; ES++) {
		_used_GHz[ES] = 0;
		//total_transfer_data_size[ES] = 0;
	}

	TD_phase(_server_list, _channel_list, _version_set, selected_set, true);
	TA_CR_phase(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, _used_GHz, _ES_count, true);
}

void TD_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, short* _selected_set, bool _flag){
	double first_GHz = 0; //lowest version만 트랜스코딩할때
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
	}
	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}

	if (_flag)
		printf("lowest version만 트랜스코딩 했을 때 %lf GHz / GHz 총 합 %lf GHz\n\n", first_GHz, GHz_limit);

	if (GHz_limit < first_GHz) {
		printf("GHz가 모자란 상황/Channel 수를 줄이거나, 엣지 수를 늘릴 것\n");
		exit(0);
	}

	//나중에 각 페이즈마다 함수 생성할 것. 그래야 보는 게 편하다.
	//1. TD phase
	double total_GHz = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//selected_ES[ch] = 0;
		_selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}
	set<pair<double, pair<int, int>>> list_TD;
	//_version_set->version_set_num(N^set)으로 초기화한 상태에서 set을 내림.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int set = 1; set <= _version_set->version_set_num - 1; set++) { //소스는 전부 1080p
			double slope = (_channel_list[ch].sum_of_pwq[_version_set->version_set_num] - _channel_list[ch].sum_of_pwq[set]) / (_channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num] - _channel_list[ch].sum_of_version_set_GHz[set]);
			list_TD.insert(make_pair(slope, make_pair(ch, set)));
		}
	}

	while (list_TD.size()) {
		int ch = (*list_TD.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int set = (*list_TD.begin()).second.second; //slope가 가장 큰 것은 어떤 세트인가?

		list_TD.erase(list_TD.begin());//맨 앞 삭제함
		//int prev_엣지_node = selected_BN[channel];
		int prev_set = _selected_set[ch];
		if (_channel_list[ch].sum_of_version_set_GHz[set] < _channel_list[ch].sum_of_version_set_GHz[prev_set]) {
			double expected_total_GHz = total_GHz - _channel_list[ch].sum_of_version_set_GHz[prev_set] + _channel_list[ch].sum_of_version_set_GHz[set];
			total_GHz = expected_total_GHz;
			if (expected_total_GHz < GHz_limit) {
				break;
			}
			_selected_set[ch] = set; //210615 오류 잡음
		}
	}

	total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[_selected_set[ch]];
	}

	if (_flag)
		std::printf("=TD= total_GHz : %lf GHz, total_pwq : %lf\n", total_GHz, total_pwq);
}


void TA_CR_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, bool _flag) {
	// 2-1. TA phase
	set<pair<double, int>> remained_GHz_of_ESs_set;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		remained_GHz_of_ESs_set.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		set <pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.end();

		int ES = -1;
		double GHz = 0;

		bool is_allocated_CTS = false;
		while (true) {
			pos--;
			if (pos == remained_GHz_of_ESs_set.begin()) {
				is_allocated_CTS = true;
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[1] >= 0)) {
				break;
			}
		}

		if (!is_allocated_CTS) {
			_selected_ES[ch][1] = ES;
			_ES_count[ES]++;

			_used_GHz[ES] += _channel_list[ch].video_GHz[1];
			//total_transfer_data_size[ES] += _version_set->data_size[1];

			remained_GHz_of_ESs_set.erase(pos);
			remained_GHz_of_ESs_set.insert(make_pair(GHz - _channel_list[ch].video_GHz[1], ES));
		}
		else {
			if (_used_GHz[0] + _channel_list[ch].video_GHz[1] <= _server_list[0].processing_capacity) {
				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				_used_GHz[0] += _channel_list[ch].video_GHz[1];
				//total_transfer_data_size[0] += _version_set->data_size[1];
			}
		}
	}

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > list_TA;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = _selected_set[ch];
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // 이전에 선택한 set에서 할당했던 GHz는 전부 삭제해 준다. 
				double slope = _channel_list[ch].pwq[ver] / _channel_list[ch].video_GHz[ver];
				list_TA.insert(make_pair(slope, make_pair(ch, ver)));
			}
		}
	}
	while (list_TA.size()) {
		int ch = (*list_TA.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*list_TA.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		list_TA.erase(list_TA.begin());//맨 앞 삭제함

		set <pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.end();
		int ES = -1;
		double GHz = 0;
		bool is_allocated_CTS = false;

		while (true) {
			pos--;
			if (pos == remained_GHz_of_ESs_set.begin()) {
				is_allocated_CTS = true;
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[ver] >= 0)) {
				break;
			}
		}

		if (!is_allocated_CTS) {
			_selected_ES[ch][ver] = ES;
			_ES_count[ES]++;

			_used_GHz[ES] += _channel_list[ch].video_GHz[ver];
			//total_transfer_data_size[ES] += _version_set->data_size[ver];

			remained_GHz_of_ESs_set.erase(pos);
			remained_GHz_of_ESs_set.insert(make_pair(GHz - _channel_list[ch].video_GHz[ver], ES));
		}
		else if (_used_GHz[0] + _channel_list[ch].video_GHz[ver] <= _server_list[0].processing_capacity) {
			_selected_ES[ch][ver] = 0;
			_ES_count[0]++;
			_used_GHz[0] += _channel_list[ch].video_GHz[ver];
			//total_transfer_data_size[0] += _version_set->data_size[ver];
		}
	}

	//set 계산하기
	set_version_set(_version_set, _selected_set, _selected_ES);

	double total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[_selected_set[ch]];
	}
	double total_cost = 0;
	double remained_GHz[ES_NUM + 1]; // processing capacity[es] - _used_GHz[es] 하면 remained_GHz[es] 하면 나옴. 모든 노드의 남은 GHz 계산을 위해.
	for (int ES = 0; ES <= ES_NUM; ES++) {
		//if (_ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES]);
			remained_GHz[ES] = _server_list[ES].processing_capacity - _used_GHz[ES];
		//}
	}

	if(_flag)
		std::printf("=CA-init= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);

	// 2-2. CR phase
	if (total_cost > _cost_limit) {
		// 아님. 완전 엎어야함. migration이 아니고 ES에 할당된 version 중에서 빼야함.
		// 이 때 ES에서 뺄 때, ingesion server에 있는 버전보다 pwq가 높을 경우, 
		// (즉 CTS에 할당된 버전 중, pwq가 제일 낮은 버전과 비교한다.)
		// (CTS에 할당된 버전이 빼려는 버전보다 더 pwq가 낮을 경우, 해당 버전은 CTS에 들어가고 원래 거기 있던 버전은 빠짐.)
		// ES에서 뺀 것은 다시 CTS에 보내고, ingesion server에서 비교 버전을 완전히 뺀다.
		set<pair<double, pair<int, int>>> list_CA_reallocation;
		// slope (pwq/cost) 값 / channel-version

		set<pair<double, pair<int, int>>> pwq_of_version_in_CTS;
		// pwq 값 / channel-version
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			double cost = 0;
			for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
				if (_selected_ES[ch][ver] > 0) { //-1은 할당 안 됨, 0은 CTS;
					double slope = _channel_list[ch].pwq[ver] / calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver]);

					list_CA_reallocation.insert(make_pair(slope, make_pair(ch, ver)));
				}

				else if (_selected_ES[ch][ver] == 0) { // 0은 CTS에 할당된 값
					if (ver > 1) {
						pwq_of_version_in_CTS.insert(make_pair(_channel_list[ch].pwq[ver], make_pair(ch, ver)));
					}
				}
			}
		}


		while (list_CA_reallocation.size()) {
			int ch = (*list_CA_reallocation.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
			int ver = (*list_CA_reallocation.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
			list_CA_reallocation.erase(list_CA_reallocation.begin());//맨 앞 삭제함

			/*if (_used_GHz[selected_ES[ch][ver]] < 0) {
				cout << "error";
			}*/

			double prev_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _used_GHz[_selected_ES[ch][ver]]);
			_ES_count[_selected_ES[ch][ver]]--;

			/*if (!_ES_count[selected_ES[ch][ver]]) {
				_used_GHz[selected_ES[ch][ver]] = 0;
				total_cost -= prev_cost;
			}
			else {*/
			_used_GHz[_selected_ES[ch][ver]] -= _channel_list[ch].video_GHz[ver];
			double curr_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _used_GHz[_selected_ES[ch][ver]]);
			total_cost -= (prev_cost - curr_cost);
			//}

			//여기까지는 cost 때문에 ES에서 version 빼는 것.
			//이제 이 뺀 version을 CTS에 할당 할 수 있을지를 봐야한다.
			//정확히는, CTS의 가장 낮은 pwq를 가진 version과 pwq를 비교한다.

			int ch_in_CTS = (*pwq_of_version_in_CTS.begin()).second.first;
			int ver_in_CTS = (*pwq_of_version_in_CTS.begin()).second.second;
			double pwq_in_CTS = (*pwq_of_version_in_CTS.begin()).first;
			double video_GHz_in_CTS = _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];
			if ((pwq_in_CTS < _channel_list[ch].pwq[ver]) &&
				((_used_GHz[0] - video_GHz_in_CTS + _channel_list[ch].video_GHz[ver]) <= _server_list[0].processing_capacity)) {
				_used_GHz[0] -= video_GHz_in_CTS;
				_used_GHz[0] += _channel_list[ch].video_GHz[ver];

				pwq_of_version_in_CTS.erase(pwq_of_version_in_CTS.begin());
				pwq_of_version_in_CTS.insert(make_pair(_channel_list[ch].pwq[ver], make_pair(ch, ver)));
				//여기까지
				_selected_ES[ch][ver] = 0;
			}
			else if (ver > 1) {
				_selected_ES[ch][ver] = -1;
			}

			if (total_cost <= _cost_limit) {
				break;
			}
		}
	}
	//set 계산하기
	set_version_set(_version_set, _selected_set, _selected_ES);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[_selected_set[ch]];

		if (_selected_set[ch] == 0) {
			cout << "error\n";
		}
	}
	total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		//if (_ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES]);
			remained_GHz[ES] = _server_list[ES].processing_capacity - _used_GHz[ES];
		//}
	}

	if (_flag)
		std::printf("=최종= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);
}