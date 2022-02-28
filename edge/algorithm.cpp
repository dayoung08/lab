#include "head.h"

//�˰��� ���⼭���� ���� ¥����
void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, int _model) {
	short selected_set[NUM_OF_CHANNEL + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
	short** selected_ES;

	selected_ES = (short**)malloc(sizeof(short*) * (NUM_OF_CHANNEL + 1));
	for (int row = 1; row <= NUM_OF_CHANNEL; row++) {
		selected_ES[row] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
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
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		total_ES_GHz_limit += _server_list[ES].processing_capacity;
		total_ES_Mbps_limit += _server_list[ES].maximum_bandwidth;
	}
	double total_ES_required_GHz = 0;
	double total_ES_required_Mbps = 0;
	double total_first_pwq = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		total_first_pwq += _channel_list[ch].sum_of_pwq[1];
		total_ES_required_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
		total_ES_required_Mbps += _channel_list[ch].sum_of_version_set_Mbps[1];
	}

	printf("lowest version�� Ʈ�����ڵ� ���� �� pwq �� �� %lf\n", total_first_pwq);
	printf("lowest version�� Ʈ�����ڵ� ���� �� %lf GHz / GHz �� �� %lf GHz\n", total_ES_required_GHz, (_server_list[0].processing_capacity + total_ES_GHz_limit));
	if ((_server_list[0].processing_capacity + total_ES_GHz_limit) < total_ES_required_GHz) {
		printf("GHz�� ���ڶ� ��Ȳ/Channel ���� ���̰ų�, ���� ���� �ø� ��\n");
		exit(0);
	}
	printf("lowest version�� Ʈ�����ڵ� ���� �� %lf Mbps / Mbps �� �� %lf Mbps\n", total_ES_required_Mbps, (_server_list[0].maximum_bandwidth + total_ES_Mbps_limit));
	if ((_server_list[0].maximum_bandwidth + total_ES_Mbps_limit) < total_ES_required_Mbps) {
		printf("Mbps�� ���ڶ� ��Ȳ/Channel ���� ���̰ų�, ���� ���� �ø� ��\n");
		exit(0);
	}

	double total_GHz = 0;
	double total_pwq = 0;
	double total_cost = 0;
	bool is_turned_on_at_lowest[NUM_OF_ES + 1];
	memset(is_turned_on_at_lowest, 0, (sizeof(bool) * (NUM_OF_ES + 1)));
	for (int is_lowest_only_mode = 1; is_lowest_only_mode >= 0; is_lowest_only_mode--) { // mode = 1 : lowest version��, mode = 0; 2~N^ver ������ ����.
		if (is_lowest_only_mode)
			std::printf("[Lowest version�� �켱 �Ҵ�]\n");
		else {
			std::printf("\n[2~N^ver ������ ���� �Ҵ�]\n");
			total_ES_required_GHz = 0;
			total_ES_required_Mbps = 0;
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				total_ES_required_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
				total_ES_required_Mbps += _channel_list[ch].sum_of_version_set_Mbps[_version_set->version_set_num];
			}
		}

		//TDA_phase 
		double GHz_rate = total_ES_required_GHz / total_ES_GHz_limit;
		double Mbps_rate = total_ES_required_Mbps / total_ES_Mbps_limit;
		TDA_phase(_server_list, _channel_list, _version_set, _cost_limit, GHz_rate, Mbps_rate, selected_set, selected_ES, used_GHz, used_Mbps, ES_count, _model, is_lowest_only_mode);
		total_cost = 0;
		for (int ES = 0; ES <= NUM_OF_ES; ES++) {
			double cpu_usage_cost = calculate_ES_cpu_usage_cost(&(_server_list[ES]), used_GHz[ES], _model);
			double bandwidth_cost = calculate_ES_bandwidth_cost(&(_server_list[ES]), used_Mbps[ES], _model);

			total_cost += max(cpu_usage_cost, bandwidth_cost);
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
			is_not_success_for_lowest_allocation(selected_ES, ES_count, (total_cost >= _cost_limit));
			if (total_cost >= _cost_limit)
				exit(0);

			// TA �������� lowest version �Ҵ� ���, �½�ũ 1�� �̻� �Ҵ� �� ���� ������ on
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
		//printf("CR phase ����, current cost: %lf\n", total_cost);
		CR_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, selected_set, selected_ES, used_GHz, used_Mbps, ES_count, _model, is_turned_on_at_lowest);

		total_cost = 0;
		for (int ES = 0; ES <= NUM_OF_ES; ES++) {
			double cpu_usage_cost = calculate_ES_cpu_usage_cost(&(_server_list[ES]), used_GHz[ES], _model);
			double bandwidth_cost = calculate_ES_bandwidth_cost(&(_server_list[ES]), used_Mbps[ES], _model);

			total_cost += max(cpu_usage_cost, bandwidth_cost);
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
}

void TDA_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, double _GHz_rate, double _Mbps_rate, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool _is_lowest_only_mode) {
	// 2. TDA phase
	set<pair<double, int>, greater<pair<double, int>>> ES_sort;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		double slope;
		if (_GHz_rate > _Mbps_rate) {
			if (_model == LINEAR_MODEL) {
				slope = (_server_list[ES].processing_capacity - _used_GHz[ES]) / calculate_ES_cpu_usage_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			}
			if (_model == ONOFF_MODEL) {
				slope = _used_GHz[ES] / _server_list[ES].cpu_usage_cost_alpha;
			}
		}
		else {
			if (_model == LINEAR_MODEL) {
				slope = (_server_list[ES].maximum_bandwidth - _used_Mbps[ES]) / calculate_ES_bandwidth_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			}
			if (_model == ONOFF_MODEL) {
				slope = _used_Mbps[ES] / _server_list[ES].bandwidth_cost_alpha;
			}
		}

		ES_sort.insert(make_pair(slope, ES)); //set
	}
	//���Ͼ�� ������ ���� �����ؼ� �Ҵ��ϴ� ���� ���� ������,
	//on-off�� ������ ���� �Ҵ��ϸ� �ȵǰ�, �Ҵ� �ϴ��� ��� �Ҵ��ϰ� �ؾ���.

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > list_TA;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		if (_is_lowest_only_mode) {
			double slope;
			if (_GHz_rate > _Mbps_rate) 
				slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[1]) / _channel_list[ch].video_GHz[1];
			else
				slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[1]) / _channel_list[ch].video_Mbps[1];

			list_TA.insert(make_pair(slope, make_pair(ch, 1)));
		}
		else {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				if ((_selected_set[ch] - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // ������ ������ set���� �Ҵ��ߴ� GHz�� ���� ������ �ش�. 
					int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
					double slope;
					if (_GHz_rate > _Mbps_rate)
						slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					else
						slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_Mbps[ver];

					list_TA.insert(make_pair(slope, make_pair(ch, ver)));
				}
			}
		}
	}

	int cnt = 0;
	while (list_TA.size()) {
		int ch = (*list_TA.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int ver = (*list_TA.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
		list_TA.erase(list_TA.begin());//�� �� ������

		set<pair<double, int>>::iterator pos = ES_sort.begin();
		int ES = -1;
		double GHz = 0;
		bool is_allocated_ES = false;
		while (true) {
			if (pos == ES_sort.end()) {
				break;
			}

			ES = (*pos).second; // ���� ���� GHz�� ���� ������ �����ΰ�?
			GHz = (*pos).first; // �� ������ GHz�� ���ΰ�?

			if ((_channel_list[ch].available_server_list[ES]) && 
				(_used_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity) &&
				(_used_Mbps[ES] + _channel_list[ch].video_Mbps[ver] <= _server_list[ES].maximum_bandwidth)) {

				/*double total_cost = 0;
				for (int es = 1; es <= NUM_OF_ES; es++) {
					if (es == ES) {
						double cpu_usage_cost = calculate_ES_cpu_usage_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _model);
						double bandwidth_cost = calculate_ES_bandwidth_cost(&(_server_list[es]), _used_Mbps[es] + _channel_list[ch].video_Mbps[ver], _model);
						total_cost += max(cpu_usage_cost, bandwidth_cost);
					}
					else {
						double cpu_usage_cost = calculate_ES_cpu_usage_cost(&(_server_list[es]), _used_GHz[es], _model);
						double bandwidth_cost = calculate_ES_bandwidth_cost(&(_server_list[es]), _used_Mbps[es], _model);
						total_cost += max(cpu_usage_cost, bandwidth_cost);
					}
				}
				if (_is_lowest_only_mode && total_cost > _cost_limit) {
					pos++;
					continue;
				}*/
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
			_used_Mbps[ES] += _channel_list[ch].video_Mbps[ver];

			ES_sort.erase(pos);
			double slope;
			if (_GHz_rate > _Mbps_rate) {
				if (_model == LINEAR_MODEL) {
					slope = (_server_list[ES].processing_capacity - _used_GHz[ES]) / calculate_ES_cpu_usage_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				}
				if (_model == ONOFF_MODEL) {
					slope = _used_GHz[ES] / _server_list[ES].cpu_usage_cost_alpha;
				}
			}
			else {
				if (_model == LINEAR_MODEL) {
					slope = (_server_list[ES].maximum_bandwidth - _used_Mbps[ES]) / calculate_ES_bandwidth_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				}
				if (_model == ONOFF_MODEL) {
					slope = _used_Mbps[ES] / _server_list[ES].bandwidth_cost_alpha;
				}
			}
			ES_sort.insert(make_pair(slope, ES)); //set
		}
		else if (_used_GHz[0] + _channel_list[ch].video_GHz[ver] <= _server_list[0].processing_capacity) {
			_selected_ES[ch][ver] = 0;
			_ES_count[0]++;
			_used_GHz[0] += _channel_list[ch].video_GHz[ver];
			_used_Mbps[0] += _channel_list[ch].video_Mbps[ver];
		}
	}

	//set ����ϱ�
	//if (!_is_lowest_only_mode)
	//	set_version_set(_version_set, _selected_set, _selected_ES);
}

void CR_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool* _is_turned_on_at_lowest) {
	set<pair<double, pair<int, int>>> versions_in_CTS;

	//cost limit�� ������ �� ���� ES ���� �� �������� �����ϰ�,
	//���ŵ� �������� CTS�� �ű����, CTS capacity �Ѵ� ���� �����Ѵ�. // 20210713 �߰���.
	if (_model == LINEAR_MODEL) {
		set<pair<double, pair<int, int>>> list_CR;
		// slope (pwq/cost) �� / channel-version
		// pwq �� / channel-version
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
				if (_selected_ES[ch][ver] == 0) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					//CTS�� ����� ������ �����Ƿ�
					versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
				}
				if (_selected_ES[ch][ver] >= 1) {
					double GHz_cost = calculate_ES_cpu_usage_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver], _model);
					double Mbps_cost = calculate_ES_bandwidth_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_Mbps[ver], _model);

					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / max(GHz_cost, Mbps_cost);
					list_CR.insert(make_pair(slope, make_pair(ch, ver)));
				}
			}
		}

		while (list_CR.size()) {
			int ch_in_ES = (*list_CR.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
			int ver_in_ES = (*list_CR.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
			list_CR.erase(list_CR.begin());// list_CR�� �� �� ������

			int set_temp = _selected_set[ch_in_ES] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver_in_ES - 1)));
			double slope = (_channel_list[ch_in_ES].sum_of_pwq[_selected_set[ch_in_ES]] - _channel_list[ch_in_ES].sum_of_pwq[set_temp]) / _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			//CTS�� ����� ������ �����Ƿ�. slope�� CTS�� �� �ĺ��� ������ ���� ���� ��.
			versions_in_CTS.insert(make_pair(slope, make_pair(ch_in_ES, ver_in_ES))); //CTS�� �ӽ� �Ҵ�

			double prev_GHz_cost = calculate_ES_cpu_usage_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			double prev_Mbps_cost = calculate_ES_bandwidth_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			double prev_cost = max(prev_GHz_cost, prev_Mbps_cost);

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

			double curr_GHz_cost = calculate_ES_cpu_usage_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			double curr_Mbps_cost = calculate_ES_bandwidth_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_Mbps[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			double curr_cost = max(curr_GHz_cost, curr_Mbps_cost);
			_total_cost -= (prev_cost - curr_cost); // cost ���

			_selected_ES[ch_in_ES][ver_in_ES] = 0; //CTS�� �ӽ� �Ҵ�

			if (_total_cost <= _cost_limit) {
				break;
			}
		}
	}

	// �� �� ES���� �� ��, ingesion server�� �ִ� �������� pwq�� ���� ���,
	// (�� CTS�� �Ҵ�� ���� ��, pwq�� ���� ���� ������ ���Ѵ�.)
	// (CTS�� �Ҵ�� ������ ������ �������� �� pwq�� ���� ���, �ش� ������ CTS�� ���� ���� �ű� �ִ� ������ ����.)
	// ES���� �� ���� �ٽ� CTS�� ������, ingesion server���� �� ������ ������ ����.

	if (_model == ONOFF_MODEL) {
		double pwq[NUM_OF_ES + 1];
		memset(pwq, 0, (sizeof(double) * (NUM_OF_ES + 1)));
		// pwq �� / channel-version
		for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));

				if (_selected_ES[ch][ver] == 0) {
					double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
					//CTS�� ����� ������ �����Ƿ�
					versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
				}
				else if (_selected_ES[ch][ver] >= 1 && !_is_turned_on_at_lowest[_selected_ES[ch][ver]]) { // ES�� �Ҵ�� �������� pwq�� ���� ������
					pwq[_selected_ES[ch][ver]] += (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]);
				}
			}
		}

		set<pair<double, int>> list_CR;
		// slope (pwq/cost) �� / ES
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (!_is_turned_on_at_lowest[ES]) {
				double GHz_cost = calculate_ES_cpu_usage_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				double Mbps_cost = calculate_ES_bandwidth_cost(&(_server_list[ES]), _used_Mbps[ES], _model);
				double slope = pwq[ES] / max(GHz_cost, Mbps_cost);
				list_CR.insert(make_pair(slope, ES));
			}
		}


		int cnt = 0;
		while (list_CR.size()) {
			cnt++;
			int ES = (*list_CR.begin()).second; // slope�� ���� ū ���� � �����ΰ�?
			list_CR.erase(list_CR.begin());//�� �� ������

			double GHz_cost = calculate_ES_cpu_usage_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			double Mbps_cost = calculate_ES_bandwidth_cost(&(_server_list[ES]), _used_Mbps[ES], _model);
			double cost = max(GHz_cost, Mbps_cost);
			_ES_count[0] += _ES_count[ES];
			_ES_count[ES] = 0;
			_used_GHz[0] += _used_GHz[ES];
			_used_GHz[ES] = 0;
			_used_Mbps[0] += _used_Mbps[ES];
			_used_Mbps[ES] = 0;

			_total_cost -= cost;
			//��������� cost ������ ES�� �Ҵ�� version���� ���� ��.

			//���� ���� �ȿ� �ִ� ������ CTS�� �Ҵ��ϱ�
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == ES) {
						if (ver > 1) {
							int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
							double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
							//CTS�� ����� ������ �����Ƿ�
							versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver))); //CTS�� �ӽ� �Ҵ�
						}
						_selected_ES[ch][ver] = 0; //CTS�� �ӽ� �Ҵ�
					}
				}
			}

			if (_total_cost <= _cost_limit) {
				break;
			}
		}
	}


	//versions_in_CTS�� processing capacity�� �°� ����.
	//�� �Ʒ� �κ� onoff model�̶� �Ȱ���
	while (versions_in_CTS.size()) {
		int ch_in_CTS = (*versions_in_CTS.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int ver_in_CTS = (*versions_in_CTS.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?

		versions_in_CTS.erase(versions_in_CTS.begin());// list_CR�� �� �� ������

		if (ver_in_CTS == 1)
			printf("error\n");

		_ES_count[0]--;
		_used_GHz[0] -= _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];
		_used_Mbps[0] -= _channel_list[ch_in_CTS].video_Mbps[ver_in_CTS];
		_selected_ES[ch_in_CTS][ver_in_CTS] = -1;

		if (_used_GHz[0] <= _server_list[0].processing_capacity) {
			break;
		}
		//CTS�� ������� �������̶� ���� GHz ���� if ���� �� ��� ��
	}

	if (_used_GHz[0] > _server_list[0].processing_capacity) {
		printf("[[Error!]] CTS ������ processing capacity �ʰ�\n");
	}

	//set ����ϱ�
	//if (!is_lowest_version)
	set_version_set(_version_set, _selected_set, _selected_ES);
}
