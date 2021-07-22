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
	}

	double used_GHz[NUM_OF_ES + 1];
	short ES_count[NUM_OF_ES + 1];
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		used_GHz[ES] = 0;
	}
	memset(ES_count, 0, (sizeof(short) * (NUM_OF_ES + 1)));

	double first_GHz = 0; //lowest version만 트랜스코딩할때
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
	}
	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}
	printf("lowest version만 트랜스코딩 했을 때 %lf GHz / GHz 총 합 %lf GHz\n\n", first_GHz, GHz_limit);
	if (GHz_limit < first_GHz) {
		printf("GHz가 모자란 상황/Channel 수를 줄이거나, 엣지 수를 늘릴 것\n");
		exit(0);
	}

	TD_phase(_server_list, _channel_list, _version_set, GHz_limit, selected_set);
	double total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	std::printf("=TD= total_GHz : %lf GHz, total_pwq : %lf\n", total_GHz, total_pwq);

	TA_phase(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	double total_cost = 0;
	double remained_GHz[NUM_OF_ES + 1]; // processing capacity[es] - _used_GHz[es] 하면 remained_GHz[es] 하면 나옴. 모든 노드의 남은 GHz 계산을 위해.
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		total_cost += calculate_ES_cost(&(_server_list[ES]), used_GHz[ES], _model);
		remained_GHz[ES] = _server_list[ES].processing_capacity - used_GHz[ES];
	}
	std::printf("=TA= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);

	CR_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);

	total_GHz = 0;
	total_pwq = 0;
	int cnt = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];

		if (selected_ES[ch][1] != -1)
			cnt++;
	}

	total_cost = 0;
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		total_cost += calculate_ES_cost(&(_server_list[ES]), used_GHz[ES], _model);
		remained_GHz[ES] = _server_list[ES].processing_capacity - used_GHz[ES];
	}

	std::printf("=CR= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);
}

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

void TA_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	// 2. TA phase
	/*set<pair<double, int>> lowest_cost_of_ES;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		lowest_cost_of_ES.insert(make_pair(0, ES)); //set
	}*/
	//210721 이 부분만 이전에는 사용 GHz가 낮은걸로 했는데, cost로 변경함. 그리고 더 좋은 결과가 나옴.
	set<pair<double, int>> remained_GHz_of_ESs_set;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		remained_GHz_of_ESs_set.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
	}

	double** slopes_of_list_TA;
	slopes_of_list_TA = (double**)malloc(sizeof(double*) * (NUM_OF_CHANNEL + 1));
	for (int row = 1; row <= NUM_OF_CHANNEL; row++) {
		slopes_of_list_TA[row] = (double*)malloc(sizeof(double) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // 오리지널 버전은 트랜스코딩 안하니까
			slopes_of_list_TA[row][col] = -1;
		}
	}
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > list_TA;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		double slope = _channel_list[ch].pwq[1] / _channel_list[ch].video_GHz[1];

		//이거 pwq/GHz나 pwq/cost나 linear 모델에선 똑같고, onoff model에선 애초에 cost는 틀린거고 계산도 안됨
		list_TA.insert(make_pair(slope, make_pair(ch, 1)));
		slopes_of_list_TA[ch][1] = slope;
	}

	double total_cost = 0;
	while (list_TA.size()) { // 모든 채널은 1번을 할당해야하므로
		int ch = (*list_TA.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		list_TA.erase(list_TA.begin());//맨 앞 삭제함

		//set <pair<double, int>>::iterator pos = lowest_cost_of_ES.begin();
		set<pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.begin();

		int ES = -1;
		double GHz = 0;

		bool is_allocated_ES = false;
		while (true) {
			//if (pos == lowest_cost_of_ES.end()) {
			if (pos == remained_GHz_of_ESs_set.end()) {
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?

			if ((_channel_list[ch].available_server_list[ES]) && (_used_GHz[ES] + _channel_list[ch].video_GHz[1] <= _server_list[ES].processing_capacity)) {
				is_allocated_ES = true;
				break;
			}

			pos++;
		}

		if (is_allocated_ES) {
			_selected_ES[ch][1] = ES;
			_ES_count[ES]++;
			_used_GHz[ES] += _channel_list[ch].video_GHz[1];

			//lowest_cost_of_ES.erase(pos);
			//double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			//lowest_cost_of_ES.insert(make_pair(cost, ES));
			remained_GHz_of_ESs_set.erase(pos);
			remained_GHz_of_ESs_set.insert(make_pair(GHz - _channel_list[ch].video_GHz[1], ES));
		}
		else {
			if (_used_GHz[0] + _channel_list[ch].video_GHz[1] <= _server_list[0].processing_capacity) {
				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				_used_GHz[0] += _channel_list[ch].video_GHz[1];
			}
		}
	}
	//1번 할당 완료

	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		int set = _selected_set[ch];
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // 이전에 선택한 set에서 할당했던 GHz는 전부 삭제해 준다. 
				//double slope = _channel_list[ch].pwq[ver] / _channel_list[ch].video_GHz[ver];
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
				double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];

				//이거 pwq/GHz나 pwq/cost나 linear 모델에선 똑같고, onoff model에선 애초에 cost는 틀린거고 계산도 안됨
				list_TA.insert(make_pair(slope, make_pair(ch, ver)));
				slopes_of_list_TA[ch][ver] = slope;
			}
		}
	}
	while (list_TA.size()) {
		int ch = (*list_TA.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*list_TA.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		list_TA.erase(list_TA.begin());//맨 앞 삭제함

		//set <pair<double, int>>::iterator pos = lowest_cost_of_ES.begin();
		set<pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.begin();
		int ES = -1;
		double GHz = 0;
		bool is_allocated_ES = false;
		while (true) {
			//if (pos == lowest_cost_of_ES.end()) {
			if (pos == remained_GHz_of_ESs_set.end()) {
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) && (_used_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity)) {
				//210715 버그 드디어 찾음
				is_allocated_ES = true;
				break;
			}

			pos++;
		}

		if (is_allocated_ES) {
			_selected_ES[ch][ver] = ES;
			_ES_count[ES]++;
			_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

			//lowest_cost_of_ES.erase(pos);
			//double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			//lowest_cost_of_ES.insert(make_pair(cost, ES));
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
	set_version_set(_version_set, _selected_set, _selected_ES);
}

//3. CR phase
void CR_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	if (_total_cost > _cost_limit) {
		set<pair<double, pair<int, int>>> versions_in_CTS;

		//cost limit를 만족할 때 까지 ES 에서 각 버전들을 제거하고,
		//제거된 버전들을 CTS로 옮긴다음, CTS capacity 넘는 것을 제거한다. // 20210713 추가함.
		if (_model == CPU_USAGE_MODEL) {
			set<pair<double, pair<int, int>>> list_CR;
			// slope (pwq/cost) 값 / channel-version
			// pwq 값 / channel-version
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == 0) { // 20210713 수정함.
						if (ver > 1) {
							int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
							double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
							versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
						}
					}
					if (_selected_ES[ch][ver] >= 1) {
						int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
						double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver], _model);
						list_CR.insert(make_pair(slope, make_pair(ch, ver)));
					}
					//여기까지 수정
				}
			}

			while (list_CR.size()) {
				int ch_in_ES = (*list_CR.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
				int ver_in_ES = (*list_CR.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
				//double slope = (*list_CR.begin()).first;
				list_CR.erase(list_CR.begin());// list_CR의 맨 앞 삭제함

				double prev_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);

				if (ver_in_ES > 1) {
					int set_temp = _selected_set[ch_in_ES] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver_in_ES - 1)));
					double slope = (_channel_list[ch_in_ES].sum_of_pwq[_selected_set[ch_in_ES]] - _channel_list[ch_in_ES].sum_of_pwq[set_temp]) / _channel_list[ch_in_ES].video_GHz[ver_in_ES];
					versions_in_CTS.insert(make_pair(slope, make_pair(ch_in_ES, ver_in_ES))); //CTS에 임시 할당
				}

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
			// pwq 값 / channel-version
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == 0) { // 20210713 수정함.
						int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
						double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
						versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
					}
					else if (_selected_ES[ch][ver] >= 1) { // ES에 할당된 버전들은 pwq의 합을 구해줌
						pwq[_selected_ES[ch][ver]] += _channel_list[ch].pwq[ver];
					}
				}
			}

			set<pair<double, int>> list_CR;
			// slope (pwq/cost) 값 / ES
			for (int ES = 1; ES <= NUM_OF_ES; ES++) {
				double slope = pwq[ES] / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				list_CR.insert(make_pair(slope, ES));
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

				for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
					for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
						if (_selected_ES[ch][ver] == ES) {
							//double slope = _channel_list[ch].pwq[ver] / _channel_list[ch].video_GHz[ver];
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
	}
	//set 계산하기
	set_version_set(_version_set, _selected_set, _selected_ES);
}