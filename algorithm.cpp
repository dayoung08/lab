#include "head.h"

//알고리즘 여기서부터 이제 짜야함
void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, int _model) {
	short selected_set[NUM_OF_CHANNEL + 1]; // 각 채널에서 사용하는 비트레이트 set
	short** selected_ES;

	selected_ES = (short**)malloc(sizeof(short*) * (NUM_OF_CHANNEL + 1));
	for (int row = 1; row <= NUM_OF_CHANNEL; row++) {
		selected_ES[row] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // 오리지널 버전은 트랜스코딩 안하니까
			selected_ES[row][col] = -1;
		}
		selected_set[row] = _version_set->version_set_num;
	}

	double used_GHz[NUM_OF_ES + 1];
	int ES_count[NUM_OF_ES + 1];
	memset(used_GHz, 0, (sizeof(double) * (NUM_OF_ES + 1)));
	memset(ES_count, 0, (sizeof(int) * (NUM_OF_ES + 1)));

	double first_pwq = 0;
	double first_GHz = 0; //lowest version만 트랜스코딩할때
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
		first_pwq += _channel_list[ch].sum_of_pwq[1];
	}
	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}
	printf("lowest version만 트랜스코딩 했을 때 pwq 총 합 %lf\n", first_pwq);
	printf("lowest version만 트랜스코딩 했을 때 %lf GHz / GHz 총 합 %lf GHz\n\n", first_GHz, GHz_limit);
	if (GHz_limit < first_GHz) {
		printf("GHz가 모자란 상황/Channel 수를 줄이거나, 엣지 수를 늘릴 것\n");
		exit(0);
	}

	//TD_phase(_server_list, _channel_list, _version_set, GHz_limit, selected_set);
	double total_GHz = 0;
	double total_pwq = 0;
	double total_cost = 0;

	/*for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	std::printf("=TD= total_GHz : %lf GHz, total_pwq : %lf\n", total_GHz, total_pwq);*/

	bool is_turn_on[NUM_OF_ES + 1];
	memset(is_turn_on, 0, (sizeof(bool) * (NUM_OF_ES + 1)));
	for (int is_lowest_only_mode = 1; is_lowest_only_mode >= 0; is_lowest_only_mode--) { // mode = 1 : lowest version만, mode = 0; 2~N^ver 버전들 전부.
		if (is_lowest_only_mode)
			std::printf("[Lowest version만 우선 할당]\n");
		else
			std::printf("[2~N^ver 버전들 전부 할당]\n");

		//TA_phase 
		TA_phase(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model, is_lowest_only_mode);
		total_cost = 0;
		for (int ES = 0; ES <= NUM_OF_ES; ES++) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), used_GHz[ES], _model);
		}

		total_GHz = 0;
		total_pwq = 0;
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
			total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
		}
		std::printf("=TA= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);

		if (is_lowest_only_mode) {
			int alloc_cnt = 0;
			for (int ES = 0; ES <= NUM_OF_ES; ES++) {
				alloc_cnt += ES_count[ES];
			}
			if (alloc_cnt < NUM_OF_CHANNEL) {
				std::printf("%d 채널의 lowest version이 할당되지 않는 문제 발생, cost budget을 높일 것.\n\n", NUM_OF_CHANNEL - alloc_cnt);
			}
			std::printf("lowest version cost : %lf $\n\n", total_cost);

			// TA 페이즈의 lowest version 할당 결과, 태스크 1개 이상 할당 된 것이 있으면 on
			if (_model == ONOFF_MODEL) {
				for (int ES = 0; ES <= NUM_OF_ES; ES++) {
					if (!is_turn_on[ES] && used_GHz[ES])
						is_turn_on[ES] = true;
				}
			}
		}
	}

	//CR_phase
	if (total_cost > _cost_limit) {
		//printf("CR phase 진입, current cost: %lf\n", total_cost);
		CR_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model, is_turn_on);

		total_cost = 0;
		for (int ES = 0; ES <= NUM_OF_ES; ES++) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), used_GHz[ES], _model);
			//remained_GHz[ES] = _server_list[ES].processing_capacity - used_GHz[ES];
		}

		total_GHz = 0;
		total_pwq = 0;
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
			total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
		}
		std::printf("=CR= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n\n", total_GHz, total_pwq, total_cost);
	}
}
/*
void TD_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _GHz_limit, short* _selected_set) {
	//나중에 각 페이즈마다 함수 생성할 것. 그래야 보는 게 편하다.
	//1. TD phase
	double total_GHz = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		//selected_ES[ch] = 0;
		_selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}

	set<pair<double, pair<int, int>>> list_TD;
	//_version_set->version_set_num(N^set)으로 초기화한 상태에서 set을 내림.
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		for (int set = 1; set <= _version_set->version_set_num - 1; set++) { //소스는 전부 1080p
			double slope = (_channel_list[ch].sum_of_pwq[_version_set->version_set_num] - _channel_list[ch].sum_of_pwq[set]) / (_channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num] - _channel_list[ch].sum_of_version_set_GHz[set]);
			list_TD.insert(make_pair(slope, make_pair(ch, set)));
		}
	}

	while (list_TD.size()) {
		int ch = (*list_TD.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int set = (*list_TD.begin()).second.second; //slope가 가장 큰 것은 어떤 세트인가?

		list_TD.erase(list_TD.begin());//맨 앞 삭제함
		int prev_set = _selected_set[ch];
		if (_channel_list[ch].sum_of_version_set_GHz[set] < _channel_list[ch].sum_of_version_set_GHz[prev_set]) {
			double expected_total_GHz = total_GHz - _channel_list[ch].sum_of_version_set_GHz[prev_set] + _channel_list[ch].sum_of_version_set_GHz[set];
			total_GHz = expected_total_GHz;
			if (expected_total_GHz < _GHz_limit) {
				break;
			}
			_selected_set[ch] = set;
		}
	}
}
*/
void TA_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, int* _ES_count, int _model, bool _is_lowest_only_mode) {
	// 2. TA phase
	set<pair<double, int>, greater<pair<double, int>>> ES_sort;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		double slope;
		if (_model == CPU_USAGE_MODEL || _model == STEP_MODEL) {
			if (_used_GHz[ES])
				slope = (_server_list[ES].processing_capacity - _used_GHz[ES]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			else
				slope = _server_list[ES].processing_capacity;
		}
		if (_model == ONOFF_MODEL) {
			if (_used_GHz[ES])
				slope = _used_GHz[ES] / _server_list[ES].cost_alpha;
			else
				slope = 1 / _server_list[ES].cost_alpha;
		}

		ES_sort.insert(make_pair(slope, ES)); //set
	}
	//리니어는 엣지를 골고루 선택해서 할당하는 것이 권장 되지만,
	//on-off는 엣지를 골고루 할당하면 안되고, 할당 하던거 계속 할당하게 해야함.

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > list_TA;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		if (_is_lowest_only_mode) {
			double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[1]) / _channel_list[ch].video_GHz[1];
			list_TA.insert(make_pair(slope, make_pair(ch, 1)));
		}
		else {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				if ((_selected_set[ch] - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // 이전에 선택한 set에서 할당했던 GHz는 전부 삭제해 준다. 
					int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];

					list_TA.insert(make_pair(slope, make_pair(ch, ver)));
				}
			}
		}
	}

	int cnt = 0;
	while (list_TA.size()) {
		int ch = (*list_TA.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*list_TA.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		list_TA.erase(list_TA.begin());//맨 앞 삭제함

		set<pair<double, int>>::iterator pos = ES_sort.begin();
		int ES = -1;
		double GHz = 0;
		bool is_allocated_ES = false;
		while (true) {
			if (pos == ES_sort.end()) {
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) && (_used_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity)) {
				is_allocated_ES = true;
				break;
			}

			pos++;
		}

		if (is_allocated_ES) {
			cnt++;
			_selected_ES[ch][ver] = ES;
			_ES_count[ES]++;
			_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

			ES_sort.erase(pos);
			double slope;
			if (_model == CPU_USAGE_MODEL || _model == STEP_MODEL) {
				if (_used_GHz[ES])
					slope = (_server_list[ES].processing_capacity - _used_GHz[ES]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				else
					slope = _server_list[ES].processing_capacity;
			}
			if (_model == ONOFF_MODEL) {
				if (_used_GHz[ES])
					slope = _used_GHz[ES] / _server_list[ES].cost_alpha;
				else
					slope = 1 / _server_list[ES].cost_alpha;
			}
			ES_sort.insert(make_pair(slope, ES)); //set
		}
		else if (_used_GHz[0] + _channel_list[ch].video_GHz[ver] <= _server_list[0].processing_capacity) {
			_selected_ES[ch][ver] = 0;
			_ES_count[0]++;
			_used_GHz[0] += _channel_list[ch].video_GHz[ver];
		}
	}

	//set 계산하기
	if (!_is_lowest_only_mode)
		set_version_set(_version_set, _selected_set, _selected_ES);
}

//3. CR phase
void CR_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, int* _ES_count, int _model, bool* _turn_on_at_lowest) {
	set<pair<double, pair<int, int>>> versions_in_CTS;

	//cost limit를 만족할 때 까지 ES 에서 각 버전들을 제거하고,
	//제거된 버전들을 CTS로 옮긴다음, CTS capacity 넘는 것을 제거한다. // 20210713 추가함.
	if (_model == CPU_USAGE_MODEL || _model == STEP_MODEL) {
		set<pair<double, pair<int, int>>> list_CR;
		// slope (pwq/cost) 값 / channel-version
		// pwq 값 / channel-version
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
				if (_selected_ES[ch][ver] == 0) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
				}
				if (_selected_ES[ch][ver] >= 1) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver], _model);
					list_CR.insert(make_pair(slope, make_pair(ch, ver)));
				}
			}
		}

		while (list_CR.size()) {
			int ch_in_ES = (*list_CR.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
			int ver_in_ES = (*list_CR.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
			list_CR.erase(list_CR.begin());// list_CR의 맨 앞 삭제함

			int set_temp = _selected_set[ch_in_ES] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver_in_ES - 1)));
			double slope = (_channel_list[ch_in_ES].sum_of_pwq[_selected_set[ch_in_ES]] - _channel_list[ch_in_ES].sum_of_pwq[set_temp]) / _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			versions_in_CTS.insert(make_pair(slope, make_pair(ch_in_ES, ver_in_ES))); //CTS에 임시 할당

			double prev_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			_ES_count[_selected_ES[ch_in_ES][ver_in_ES]]--;
			_ES_count[0]++;
			if (_ES_count[_selected_ES[ch_in_ES][ver_in_ES]]) {
				_used_GHz[_selected_ES[ch_in_ES][ver_in_ES]] -= _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			}
			else {
				_used_GHz[_selected_ES[ch_in_ES][ver_in_ES]] = 0;
			}
			_used_GHz[0] += _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			double curr_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			_total_cost -= (prev_cost - curr_cost); // cost 계산

			_selected_ES[ch_in_ES][ver_in_ES] = 0; //CTS에 임시 할당

			if (_total_cost <= _cost_limit) {
				break;
			}
		}
	}

	// 이 때 ES에서 뺄 때, ingesion server에 있는 버전보다 pwq가 높을 경우,
	// (즉 CTS에 할당된 버전 중, pwq가 제일 낮은 버전과 비교한다.)
	// (CTS에 할당된 버전이 빼려는 버전보다 더 pwq가 낮을 경우, 해당 버전은 CTS에 들어가고 원래 거기 있던 버전은 빠짐.)
	// ES에서 뺀 것은 다시 CTS에 보내고, ingesion server에서 비교 버전을 완전히 뺀다.

	if (_model == ONOFF_MODEL) {
		double pwq[NUM_OF_ES + 1];
		memset(pwq, 0, (sizeof(double) * (NUM_OF_ES + 1)));
		// pwq 값 / channel-version
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));

				if (_selected_ES[ch][ver] == 0) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
				}
				else if (_selected_ES[ch][ver] >= 1 && !_turn_on_at_lowest[_selected_ES[ch][ver]]) { // ES에 할당된 버전들은 pwq의 합을 구해줌
					pwq[_selected_ES[ch][ver]] += (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]);
				}
			}
		}

		set<pair<double, int>> list_CR;
		// slope (pwq/cost) 값 / ES
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (!_turn_on_at_lowest[ES]) {
				double slope = pwq[ES] / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				list_CR.insert(make_pair(slope, ES));
			}
		}


		int cnt = 0;
		while (list_CR.size()) {
			cnt++;
			int ES = (*list_CR.begin()).second; // slope가 가장 큰 것은 어떤 엣지인가?
			list_CR.erase(list_CR.begin());//맨 앞 삭제함

			double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			_ES_count[0] += _ES_count[ES];
			_ES_count[ES] = 0;
			_used_GHz[0] += _used_GHz[ES];
			_used_GHz[ES] = 0;
			_total_cost -= cost;
			//여기까지는 cost 때문에 ES에 할당된 version들을 빼는 것.

			//뺐던 엣지 안에 있는 버전들 CTS에 할당하기
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == ES) {
						int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
						double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
						versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver))); //CTS에 임시 할당
						_selected_ES[ch][ver] = 0; //CTS에 임시 할당
					}
				}
			}

			if (_total_cost <= _cost_limit) {
				break;
			}
		}
	}


	//versions_in_CTS의 processing capacity에 맞게 삭제.
	//이 아래 부분 onoff model이랑 똑같음
	while (versions_in_CTS.size()) {
		int ch_in_CTS = (*versions_in_CTS.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver_in_CTS = (*versions_in_CTS.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?

		versions_in_CTS.erase(versions_in_CTS.begin());// list_CR의 맨 앞 삭제함

		_ES_count[0]--;
		_used_GHz[0] -= _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];
		_selected_ES[ch_in_CTS][ver_in_CTS] = -1;

		if (_used_GHz[0] <= _server_list[0].processing_capacity) {
			break;
		}
	}

	if (_used_GHz[0] > _server_list[0].processing_capacity) {
		printf("[[Error!]] CTS 서버의 processing capacity 초과\n");
	}

	//set 계산하기
	//if (!is_lowest_version)
	set_version_set(_version_set, _selected_set, _selected_ES);
}