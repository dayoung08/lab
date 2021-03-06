#include "head.h"

//알고리즘 여기서부터 이제 짜야함
void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, int _model, bool _bandwidth_apply_flag) {
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
	double used_Mbps[NUM_OF_ES + 1];
	int ES_count[NUM_OF_ES + 1];
	memset(used_GHz, 0, (sizeof(double) * (NUM_OF_ES + 1)));
	memset(used_Mbps, 0, (sizeof(double) * (NUM_OF_ES + 1)));
	memset(ES_count, 0, (sizeof(int) * (NUM_OF_ES + 1)));

	double total_ES_GHz_limit = 0;
	double total_ES_Mbps_limit = 0;
	double total_ES_required_GHz = 0;
	double total_ES_required_Mbps = 0;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		total_ES_GHz_limit += _server_list[ES].processing_capacity;
		total_ES_Mbps_limit += _server_list[ES].maximum_bandwidth;
	}
	double total_first_pwq = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		total_first_pwq += _channel_list[ch].sum_of_pwq[1];
		total_ES_required_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
		total_ES_required_Mbps += _channel_list[ch].sum_of_version_set_Mbps[1];
	}

	printf("lowest version만 트랜스코딩 했을 때 pwq 총 합 %lf\n", total_first_pwq);
	printf("lowest version만 트랜스코딩 했을 때 %lf GHz / GHz 총 합 %lf GHz\n", total_ES_required_GHz, (_server_list[0].processing_capacity + total_ES_GHz_limit));
	if ((_server_list[0].processing_capacity + total_ES_GHz_limit) < total_ES_required_GHz) {
		printf("GHz가 모자란 상황/Channel 수를 줄이거나, 엣지 수를 늘릴 것\n");
		exit(0);
	}
	printf("lowest version만 트랜스코딩 했을 때 %lf Mbps / Mbps 총 합 %lf Mbps\n", total_ES_required_Mbps, (_server_list[0].maximum_bandwidth + total_ES_Mbps_limit));
	if ((_server_list[0].maximum_bandwidth + total_ES_Mbps_limit) < total_ES_required_Mbps) {
		printf("Mbps가 모자란 상황/Channel 수를 줄이거나, 엣지 수를 늘릴 것\n");
		exit(0);
	}
	//TD_phase(_server_list, _channel_list, _version_set, selected_set);

	double total_GHz = 0;
	double total_pwq = 0;
	double total_cost = 0;
	bool is_turned_on_at_lowest[NUM_OF_ES + 1];
	memset(is_turned_on_at_lowest, 0, (sizeof(bool) * (NUM_OF_ES + 1)));
	for (int is_lowest_only_mode = 1; is_lowest_only_mode >= 0; is_lowest_only_mode--) { // mode = 1 : lowest version만, mode = 0; 2~N^ver 버전들 전부.
		if (is_lowest_only_mode)
			std::printf("[Lowest version만 우선 할당]\n");
		else {
			std::printf("\n[2~N^ver 버전들 전부 할당]\n");
		}

		//TDA_phase _bandwidth_apply_flag
		if(!_bandwidth_apply_flag)
			TDA_phase(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, used_Mbps, ES_count, _model, is_lowest_only_mode);
		else
			TDA_phase_for_bandwidth(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, used_Mbps, ES_count, _model, is_lowest_only_mode);

		total_cost = 0;
		for (int ES = 0; ES <= NUM_OF_ES; ES++) {
			double cost = calculate_ES_cost(&(_server_list[ES]), used_GHz[ES], used_Mbps[ES], _model);
			total_cost += cost;
		}
		if (!is_lowest_only_mode) {
			set_version_set(_version_set, selected_set, selected_ES);
		}

		total_GHz = 0;
		total_pwq = 0;
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			if (is_lowest_only_mode)
				total_pwq += _channel_list[ch].sum_of_pwq[1];
			else
				total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
			total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		}
		std::printf("=TDA= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);

		if (is_lowest_only_mode) {
			is_success_for_lowest_allocation(selected_ES, ES_count);
			//if (!is_success_for_lowest_allocation(selected_ES, ES_count)) {
				//exit(0);
			//}
			// TA 페이즈의 lowest version 할당 결과, 태스크 1개 이상 할당 된 것이 있으면 on
			if (_model == ONOFF_MODEL) {
				for (int ES = 0; ES <= NUM_OF_ES; ES++) {
					if (!is_turned_on_at_lowest[ES] && used_GHz[ES])
						is_turned_on_at_lowest[ES] = true;
				}
			}
		}
	}

	//CR_phase
	if (total_cost > _cost_limit) {
		//printf("CR phase 진입, current cost: %lf\n", total_cost);
		if (!_bandwidth_apply_flag)
			CR_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, selected_set, selected_ES, used_GHz, used_Mbps, ES_count, _model, is_turned_on_at_lowest);
		else
			CR_phase_for_bandwidth(_server_list, _channel_list, _version_set, total_cost, _cost_limit, selected_set, selected_ES, used_GHz, used_Mbps, ES_count, _model, is_turned_on_at_lowest);

		set_version_set(_version_set, selected_set, selected_ES);
	}
	total_cost = 0;
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		double cost = calculate_ES_cost(&(_server_list[ES]), used_GHz[ES], used_Mbps[ES], _model);
		total_cost += cost;
	}

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	//std::printf("=CR= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n\n", total_GHz, total_pwq, total_cost);
	std::printf("%lf\n", total_pwq);
}

void TDA_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool _is_lowest_only_mode) {
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > task_list_TDA;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		if (_is_lowest_only_mode) {
			double slope = slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[1]) / _channel_list[ch].video_GHz[1];
			task_list_TDA.insert(make_pair(slope, make_pair(ch, 1)));
		}
		else { //bandwidth 제약 적용
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				if ((_selected_set[ch] - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // 이전에 선택한 set에서 할당했던 GHz는 전부 삭제해 준다. 
					int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					task_list_TDA.insert(make_pair(slope, make_pair(ch, ver)));
				}
			} 
		}
	}

	int cnt = 0;
	while (task_list_TDA.size()) {
		int ch = (*task_list_TDA.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver = (*task_list_TDA.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		task_list_TDA.erase(task_list_TDA.begin());//맨 앞 삭제함

		set<pair<double, int>, greater<pair<double, int>>> ES_list;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			double slope;
			if (_model == LINEAR_MODEL) {
				slope = (_server_list[ES].processing_capacity - _used_GHz[ES] - _channel_list[ch].video_GHz[ver]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _used_Mbps[ES] + _channel_list[ch].video_Mbps[ver], _model);
			}
			if (_model == ONOFF_MODEL) {
				slope = (_used_GHz[ES] + _channel_list[ch].video_GHz[ver]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _used_Mbps[ES] + _channel_list[ch].video_Mbps[ver], _model);
			}
			ES_list.insert(make_pair(slope, ES));
		}
		//리니어는 엣지를 골고루 선택해서 할당하는 것이 권장 되지만,
		//on-off는 엣지를 골고루 할당하면 안되고, 할당 하던거 계속 할당하게 해야함.

		set<pair<double, int>>::iterator pos = ES_list.begin();
		int ES = -1;
		double GHz = 0;
		bool is_allocated_ES = false;
		while (true) {
			if (pos == ES_list.end()) {
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) &&
				(_used_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity) &&
				(_used_Mbps[ES] + _channel_list[ch].video_Mbps[ver] <= _server_list[ES].maximum_bandwidth)) {

				double total_cost = 0;
				for (int es = 1; es <= NUM_OF_ES; es++) {
					if (es == ES) {
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _used_Mbps[es] + _channel_list[ch].video_Mbps[ver], _model);
					}
					else {
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _used_Mbps[es], _model);
					}
				}
				if (_is_lowest_only_mode && total_cost > _cost_limit) {
					pos++;
					continue;
				}
				else {
					is_allocated_ES = true;
					break;
				}
			}
			else
				pos++;
		}

		if (is_allocated_ES) {
			cnt++;
			_selected_ES[ch][ver] = ES;
			_ES_count[ES]++;
			_used_GHz[ES] += _channel_list[ch].video_GHz[ver];
			_used_Mbps[ES] += _channel_list[ch].video_Mbps[ver];
		}
		else if (_used_GHz[0] + _channel_list[ch].video_GHz[ver] <= _server_list[0].processing_capacity) {
			_selected_ES[ch][ver] = 0;
			_ES_count[0]++;
			_used_GHz[0] += _channel_list[ch].video_GHz[ver];
			_used_Mbps[0] += _channel_list[ch].video_Mbps[ver];
		}
	}

	//set 계산하기
	//if (!_is_lowest_only_mode)
	//	set_version_set(_version_set, _selected_set, _selected_ES);
}

void CR_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool* _is_turned_on_at_lowest) {
	set<pair<double, pair<int, int>>> task_list_in_CTS;

	//cost limit를 만족할 때 까지 ES 에서 각 버전들을 제거하고,
	//제거된 버전들을 CTS로 옮긴다음, CTS capacity 넘는 것을 제거한다. // 20210713 추가함.
	if (_model == LINEAR_MODEL) {
		set<pair<double, pair<int, int>>> list_CR;
		// slope (pwq/cost) 값 / channel-version
		// pwq 값 / channel-version
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
				if (_selected_ES[ch][ver] == 0) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					//CTS의 밴드윗 제한은 없으므로
					task_list_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
				}
				if (_selected_ES[ch][ver] >= 1) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver], _channel_list[ch].video_Mbps[ver], _model);
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
			//CTS의 밴드윗 제한은 없으므로. slope는 CTS에 들어간 후보군 선택을 위해 계산된 것.
			task_list_in_CTS.insert(make_pair(slope, make_pair(ch_in_ES, ver_in_ES))); //CTS에 임시 할당

			double prev_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]], _model);

			_ES_count[_selected_ES[ch_in_ES][ver_in_ES]]--;
			_ES_count[0]++;
			if (_ES_count[_selected_ES[ch_in_ES][ver_in_ES]]) {
				_used_GHz[_selected_ES[ch_in_ES][ver_in_ES]] -= _channel_list[ch_in_ES].video_GHz[ver_in_ES];
				_used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]] -= _channel_list[ch_in_ES].video_Mbps[ver_in_ES];
			}
			else {
				_used_GHz[_selected_ES[ch_in_ES][ver_in_ES]] = 0;
				_used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]] = 0;
			}
			_used_GHz[0] += _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			_used_Mbps[0] += _channel_list[ch_in_ES].video_Mbps[ver_in_ES];

			double curr_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			
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
					//CTS의 밴드윗 제한은 없으므로
					task_list_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
				}
				else if (_selected_ES[ch][ver] >= 1 && !_is_turned_on_at_lowest[_selected_ES[ch][ver]]) { // ES에 할당된 버전들은 pwq의 합을 구해줌
					pwq[_selected_ES[ch][ver]] += (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]);
				}
			}
		}

		set<pair<double, int>> list_CR;
		// slope (pwq/cost) 값 / ES
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (!_is_turned_on_at_lowest[ES]) {
				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _used_Mbps[ES], _model);
				double slope = pwq[ES] / cost;
				list_CR.insert(make_pair(slope, ES));
			}
		}


		int cnt = 0;
		while (list_CR.size()) {
			cnt++;
			int ES = (*list_CR.begin()).second; // slope가 가장 큰 것은 어떤 엣지인가?
			list_CR.erase(list_CR.begin());//맨 앞 삭제함

			double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _used_Mbps[ES], _model);
			_ES_count[0] += _ES_count[ES];
			_ES_count[ES] = 0;
			_used_GHz[0] += _used_GHz[ES];
			_used_GHz[ES] = 0;
			_used_Mbps[0] += _used_Mbps[ES];
			_used_Mbps[ES] = 0;

			_total_cost -= cost;
			//여기까지는 cost 때문에 ES에 할당된 version들을 빼는 것.

			//뺐던 엣지 안에 있는 버전들 CTS에 할당하기
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == ES) {
						if (ver > 1) {
							int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
							double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
							//CTS의 밴드윗 제한은 없으므로
							task_list_in_CTS.insert(make_pair(slope, make_pair(ch, ver))); //CTS에 임시 할당
						}
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
	while (task_list_in_CTS.size()) {
		int ch_in_CTS = (*task_list_in_CTS.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver_in_CTS = (*task_list_in_CTS.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?

		task_list_in_CTS.erase(task_list_in_CTS.begin());// list_CR의 맨 앞 삭제함

		if (ver_in_CTS == 1)
			printf("error\n");

		_ES_count[0]--;
		_used_GHz[0] -= _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];
		_used_Mbps[0] -= _channel_list[ch_in_CTS].video_Mbps[ver_in_CTS];
		_selected_ES[ch_in_CTS][ver_in_CTS] = -1;

		if (_used_GHz[0] <= _server_list[0].processing_capacity) {
			break;
		}
		//CTS의 밴드윗은 무제한이라 위의 GHz 관련 if 같은 것 없어도 됨
	}

	if (_used_GHz[0] > _server_list[0].processing_capacity) {
		printf("[[Error!]] CTS 서버의 processing capacity 초과\n");
	}

	//set 계산하기
	//if (!is_lowest_version)
}


/*void TD_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, short* _selected_set) {
	//1. TD phase
	double total_GHz = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		_selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}
	set<pair<double, pair<int, int>>> list_TD;
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
			if (expected_total_GHz < sum_of_processing_capacity) {
				break;
			}
			_selected_set[ch] = set;
		}
	}
}
*/


void TDA_phase_for_bandwidth(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool _is_lowest_only_mode) {
	bool** is_processed = (bool**)malloc(sizeof(bool*) * (NUM_OF_CHANNEL + 1));
	for (int row = 1; row <= NUM_OF_CHANNEL; row++) {
		is_processed[row] = (bool*)malloc(sizeof(bool) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // 오리지널 버전은 트랜스코딩 안하니까
			is_processed[row][col] = false;
		}
	}
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > task_list_TDA_GHz;
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > task_list_TDA_Mbps;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		if (_is_lowest_only_mode) {
			double slope_GHz = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[1]) / _channel_list[ch].video_GHz[1];
			double slope_Mbps = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[1]) / _channel_list[ch].video_Mbps[1];
			task_list_TDA_GHz.insert(make_pair(slope_GHz, make_pair(ch, 1)));
			task_list_TDA_Mbps.insert(make_pair(slope_Mbps, make_pair(ch, 1)));
		}
		else { //bandwidth 제약 적용
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				if ((_selected_set[ch] - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // 이전에 선택한 set에서 할당했던 GHz는 전부 삭제해 준다. 
					int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
					double slope_GHz = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					double slope_Mbps = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_Mbps[ver];
					task_list_TDA_GHz.insert(make_pair(slope_GHz, make_pair(ch, ver)));
					task_list_TDA_Mbps.insert(make_pair(slope_Mbps, make_pair(ch, ver)));
				}
			} 
		}
	}

	int cnt = 0;
	while (task_list_TDA_GHz.size() && task_list_TDA_Mbps.size()) {
		int ch_GHz = (*task_list_TDA_GHz.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver_GHz = (*task_list_TDA_GHz.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		int ch_Mbps = (*task_list_TDA_Mbps.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver_Mbps = (*task_list_TDA_Mbps.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?

		bool continue_flag = false;
		if (is_processed[ch_GHz][ver_GHz]) { // 이미 할당 완료된 task
			task_list_TDA_GHz.erase(task_list_TDA_GHz.begin());//맨 앞 삭제함
			continue_flag = true;
		}
		if (is_processed[ch_Mbps][ver_Mbps]) { // 이미 할당 완료된 task
			task_list_TDA_Mbps.erase(task_list_TDA_Mbps.begin());//맨 앞 삭제함
			continue_flag = true;
		}
		if (continue_flag)
			continue;
		//할당 완료 된 것이 선택된다면 거르는 용도

		bool resource_flag;
		double sum_of_remained_GHz = 0;
		double sum_of_remained_Mbps= 0;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			sum_of_remained_GHz += (_server_list[ES].processing_capacity -_used_GHz[ES]);
			sum_of_remained_Mbps += (_server_list[ES].maximum_bandwidth - _used_Mbps[ES]);
		}

		int ch = -1;
		int ver = -1;
		if (_channel_list[ch_GHz].video_GHz[ver_GHz] / sum_of_remained_GHz > _channel_list[ch_Mbps].video_Mbps[ver_Mbps] / sum_of_remained_Mbps) {
			resource_flag = true;
		}
		else {
			resource_flag = false;
		}

		set<pair<double, int>>::iterator pos;
		set<pair<double, int>, greater<pair<double, int>>> ES_list_GHz;
		set<pair<double, int>, greater<pair<double, int>>> ES_list_Mbps;
		if (resource_flag) {
			task_list_TDA_GHz.erase(task_list_TDA_GHz.begin());//맨 앞 삭제함
			ch = ch_GHz;
			ver = ver_GHz;

			for (int ES = 1; ES <= NUM_OF_ES; ES++) {
				double slope_GHz;
				if (_model == LINEAR_MODEL) {
					slope_GHz = (_server_list[ES].processing_capacity - _used_GHz[ES] - _channel_list[ch].video_GHz[ver]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _used_Mbps[ES] + _channel_list[ch].video_Mbps[ver], _model);
				}
				if (_model == ONOFF_MODEL) {
					slope_GHz = (_used_GHz[ES] + _channel_list[ch].video_GHz[ver]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _used_Mbps[ES] + _channel_list[ch].video_Mbps[ver], _model);
				}
				ES_list_GHz.insert(make_pair(slope_GHz, ES));
			}
			pos = ES_list_GHz.begin();
		}
		else {
			task_list_TDA_Mbps.erase(task_list_TDA_Mbps.begin());//맨 앞 삭제함
			ch = ch_Mbps;
			ver = ver_Mbps;

			for (int ES = 1; ES <= NUM_OF_ES; ES++) {
				double slope_Mbps;
				if (_model == LINEAR_MODEL) {
					slope_Mbps = (_server_list[ES].maximum_bandwidth - _used_Mbps[ES] - _channel_list[ch].video_Mbps[ver]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _used_Mbps[ES] + _channel_list[ch].video_Mbps[ver], _model);
				}
				if (_model == ONOFF_MODEL) {
					slope_Mbps = (_used_Mbps[ES] + _channel_list[ch].video_Mbps[ver]) / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _used_Mbps[ES] + _channel_list[ch].video_Mbps[ver], _model);
				}
				ES_list_Mbps.insert(make_pair(slope_Mbps, ES));
			}
			//리니어는 엣지를 골고루 선택해서 할당하는 것이 권장 되지만,
			//on-off는 엣지를 골고루 할당하면 안되고, 할당 하던거 계속 할당하게 해야함.
			pos = ES_list_Mbps.begin();
		}

		int ES = -1;
		double GHz = 0;
		bool is_allocated_ES = false;
		while (true) {
			if (resource_flag && pos == ES_list_GHz.end()) {
				break;
			}
			if (!resource_flag && pos == ES_list_Mbps.end()) {
				break;
			}

			ES = (*pos).second; // 가장 남은 GHz가 많은 엣지는 무엇인가?
			GHz = (*pos).first; // 그 엣지의 GHz는 얼마인가?

			if ((_channel_list[ch].available_server_list[ES]) &&
				(_used_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity) &&
				(_used_Mbps[ES] + _channel_list[ch].video_Mbps[ver] <= _server_list[ES].maximum_bandwidth)) {

				double total_cost = 0;
				for (int es = 1; es <= NUM_OF_ES; es++) {
					if (es == ES) {
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _used_Mbps[es] + _channel_list[ch].video_Mbps[ver], _model);
					}
					else {
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _used_Mbps[es], _model);
					}
				}
				if (_is_lowest_only_mode && total_cost > _cost_limit) {
					pos++;
					continue;
				}
				else {
					is_allocated_ES = true;
					break;
				}
			}
			else
				pos++;
		}

		if (is_allocated_ES) {
			cnt++;
			_selected_ES[ch][ver] = ES;
			_ES_count[ES]++;
			_used_GHz[ES] += _channel_list[ch].video_GHz[ver];
			_used_Mbps[ES] += _channel_list[ch].video_Mbps[ver];
		}
		else if (_used_GHz[0] + _channel_list[ch].video_GHz[ver] <= _server_list[0].processing_capacity) {
			_selected_ES[ch][ver] = 0;
			_ES_count[0]++;
			_used_GHz[0] += _channel_list[ch].video_GHz[ver];
			_used_Mbps[0] += _channel_list[ch].video_Mbps[ver];
		}
		is_processed[ch][ver] = true;
	}
}


void CR_phase_for_bandwidth(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool* _is_turned_on_at_lowest) {
	bool** is_processed = (bool**)malloc(sizeof(bool*) * (NUM_OF_CHANNEL + 1));
	for (int row = 1; row <= NUM_OF_CHANNEL; row++) {
		is_processed[row] = (bool*)malloc(sizeof(bool) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // 오리지널 버전은 트랜스코딩 안하니까
			is_processed[row][col] = false;
		}
	}

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > task_list_in_CTS_GHz;
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > task_list_in_CTS_Mbps;

	//cost limit를 만족할 때 까지 ES 에서 각 버전들을 제거하고,
	//제거된 버전들을 CTS로 옮긴다음, CTS capacity 넘는 것을 제거한다. // 20210713 추가함.
	if (_model == LINEAR_MODEL) {
		set<pair<double, pair<int, int>>> list_CR;
		// slope (pwq/cost) 값 / channel-version
		// pwq 값 / channel-version
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
				if (_selected_ES[ch][ver] == 0) {
					double slope_GHz = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					double slope_Mbps = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_Mbps[ver];
					task_list_in_CTS_GHz.insert(make_pair(slope_GHz, make_pair(ch, ver)));
					task_list_in_CTS_Mbps.insert(make_pair(slope_Mbps, make_pair(ch, ver)));
				}
				if (_selected_ES[ch][ver] >= 1) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver], _channel_list[ch].video_Mbps[ver], _model);
					list_CR.insert(make_pair(slope, make_pair(ch, ver)));
				}
			}
		}

		while (list_CR.size()) {
			int ch_in_ES = (*list_CR.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
			int ver_in_ES = (*list_CR.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
			list_CR.erase(list_CR.begin());// list_CR의 맨 앞 삭제함

			int set_temp = _selected_set[ch_in_ES] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver_in_ES - 1)));
			double slope_GHz = (_channel_list[ch_in_ES].sum_of_pwq[_selected_set[ch_in_ES]] - _channel_list[ch_in_ES].sum_of_pwq[set_temp]) / _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			double slope_Mbps = (_channel_list[ch_in_ES].sum_of_pwq[_selected_set[ch_in_ES]] - _channel_list[ch_in_ES].sum_of_pwq[set_temp]) / _channel_list[ch_in_ES].video_Mbps[ver_in_ES];//CTS에 임시 할당
			task_list_in_CTS_GHz.insert(make_pair(slope_GHz, make_pair(ch_in_ES, ver_in_ES)));
			task_list_in_CTS_Mbps.insert(make_pair(slope_Mbps, make_pair(ch_in_ES, ver_in_ES)));

			double prev_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]], _model);

			_ES_count[_selected_ES[ch_in_ES][ver_in_ES]]--;
			_ES_count[0]++;
			if (_ES_count[_selected_ES[ch_in_ES][ver_in_ES]]) {
				_used_GHz[_selected_ES[ch_in_ES][ver_in_ES]] -= _channel_list[ch_in_ES].video_GHz[ver_in_ES];
				_used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]] -= _channel_list[ch_in_ES].video_Mbps[ver_in_ES];
			}
			else {
				_used_GHz[_selected_ES[ch_in_ES][ver_in_ES]] = 0;
				_used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]] = 0;
			}
			_used_GHz[0] += _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			_used_Mbps[0] += _channel_list[ch_in_ES].video_Mbps[ver_in_ES];

			double curr_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]], _model);

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
					double slope_GHz = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					double slope_Mbps = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_Mbps[ver];
					task_list_in_CTS_GHz.insert(make_pair(slope_GHz, make_pair(ch, ver)));
					task_list_in_CTS_Mbps.insert(make_pair(slope_Mbps, make_pair(ch, ver)));
				}
				else if (_selected_ES[ch][ver] >= 1 && !_is_turned_on_at_lowest[_selected_ES[ch][ver]]) { // ES에 할당된 버전들은 pwq의 합을 구해줌
					pwq[_selected_ES[ch][ver]] += (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]);
				}
			}
		}

		set<pair<double, int>> list_CR;
		// slope (pwq/cost) 값 / ES
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (!_is_turned_on_at_lowest[ES]) {
				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _used_Mbps[ES], _model);
				double slope = pwq[ES] / cost;
				list_CR.insert(make_pair(slope, ES));
			}
		}

		int cnt = 0;
		while (list_CR.size()) {
			cnt++;
			int ES = (*list_CR.begin()).second; // slope가 가장 큰 것은 어떤 엣지인가?
			list_CR.erase(list_CR.begin());//맨 앞 삭제함

			double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _used_Mbps[ES], _model);
			_ES_count[0] += _ES_count[ES];
			_ES_count[ES] = 0;
			_used_GHz[0] += _used_GHz[ES];
			_used_GHz[ES] = 0;
			_used_Mbps[0] += _used_Mbps[ES];
			_used_Mbps[ES] = 0;

			_total_cost -= cost;
			//여기까지는 cost 때문에 ES에 할당된 version들을 빼는 것.

			//뺐던 엣지 안에 있는 버전들 CTS에 할당하기
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == ES) {
						if (ver > 1) {
							int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
							double slope_GHz = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
							double slope_Mbps = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_Mbps[ver];
							task_list_in_CTS_GHz.insert(make_pair(slope_GHz, make_pair(ch, ver)));
							task_list_in_CTS_Mbps.insert(make_pair(slope_Mbps, make_pair(ch, ver)));
						}
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
	while (task_list_in_CTS_GHz.size() && task_list_in_CTS_Mbps.size()) {
		int ch_GHz = (*task_list_in_CTS_GHz.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver_GHz = (*task_list_in_CTS_GHz.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?
		int ch_Mbps = (*task_list_in_CTS_Mbps.begin()).second.first; // slope가 가장 큰 것은 어떤 채널인가?
		int ver_Mbps = (*task_list_in_CTS_Mbps.begin()).second.second; // slope가 가장 큰 것은 어떤 버전인가?

		bool continue_flag = false;
		if (is_processed[ch_GHz][ver_GHz]) { // 이미 할당 완료된 task
			task_list_in_CTS_GHz.erase(task_list_in_CTS_GHz.begin());//맨 앞 삭제함
			continue_flag = true;
		}
		if (is_processed[ch_Mbps][ver_Mbps]) { // 이미 할당 완료된 task
			task_list_in_CTS_Mbps.erase(task_list_in_CTS_Mbps.begin());//맨 앞 삭제함
			continue_flag = true;
		}
		if (continue_flag)
			continue;
		//할당 완료 된 것이 선택된다면 거르는 용도

		//bool resource_flag;
		double remained_GHz = _server_list[0].processing_capacity - _used_GHz[0];
		double remained_Mbps = _server_list[0].maximum_bandwidth - _used_Mbps[0];

		int ch_in_CTS = -1;
		int ver_in_CTS = -1;
		if (_channel_list[ch_GHz].video_GHz[ver_GHz] / remained_GHz > _channel_list[ch_Mbps].video_Mbps[ver_Mbps] / remained_Mbps) {
			//resource_flag = true;
			ch_in_CTS = ch_GHz;
			ver_in_CTS = ver_GHz;
		}
		else {
			//resource_flag = false;
			ch_in_CTS = ch_Mbps;
			ver_in_CTS = ver_Mbps;
		}

		if (ver_in_CTS == 1)
			printf("error\n");

		_ES_count[0]--;
		_used_GHz[0] -= _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];
		_used_Mbps[0] -= _channel_list[ch_in_CTS].video_Mbps[ver_in_CTS];
		_selected_ES[ch_in_CTS][ver_in_CTS] = -1;
		is_processed[ch_in_CTS][ver_in_CTS] = true;

		if (_used_GHz[0] <= _server_list[0].processing_capacity) {
			break;
		}
		//CTS의 밴드윗은 무제한이라 위의 GHz 관련 if 같은 것 없어도 됨
	}

	if (_used_GHz[0] > _server_list[0].processing_capacity) {
		printf("[[Error!]] CTS 서버의 processing capacity 초과\n");
	}
}
