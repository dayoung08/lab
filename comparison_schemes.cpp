#include "head.h"

//���������� Ȯ�� �� �ʿ䰡 ���� 20210515
//�� ��Ŵ���� �����Ǿ���.

void comparison_schemes(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, int _model) {
	short selected_set[CHANNEL_NUM + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
	memset(selected_set, 0, (sizeof(short) * (NUM_OF_ES + 1)));

	short** selected_ES;//[CHANNEL_NUM + 1][VERSION_NUM]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set�� ���ϴ� �� ������ � es���� ���õǾ��°�.
	//�������� ������ Ʈ�����ڵ� ���ؼ� �迭 ũ�Ⱑ ������.

	double used_GHz[NUM_OF_ES + 1];
	short ES_count[NUM_OF_ES + 1];
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		used_GHz[ES] = 0;
	}
	memset(ES_count, 0, (sizeof(short) * (NUM_OF_ES + 1)));

	selected_ES = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		selected_ES[ch] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			selected_ES[ch][ver] = -1;
		}
	}
	if (method_index == GHz_WF_AP) {
		GHz_worst_fit_AP(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}
	if (method_index == GHz_WF_HPF) {
		GHz_worst_fit_HPF(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}
	if (method_index == GHz_WF_TD) {
		GHz_worst_fit_TD_phase(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}
	if (method_index == cost_WF_AP) {
		cost_worst_fit_AP(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}
	if (method_index == cost_WF_HPF) {
		cost_worst_fit_HPF(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}
	if (method_index == cost_WF_TD) {
		cost_worst_fit_HPF(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}
	if (method_index == TA_CR_AP) {
		TA_CR_phase_AP(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}
	if (method_index == TA_CR_HPF) {
		TA_CR_phase_HPF(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, used_GHz, ES_count, _model);
	}

	set_version_set(_version_set, selected_set, selected_ES);
	print_method(method_index, _server_list, _channel_list, _version_set, selected_set, used_GHz, ES_count, _model);
}

void print_method(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set,  short* _selected_set, double* _used_GHz, short* _ES_count, int _model) {
	if (method_index == GHz_WF_AP) {
		printf("<<GHz_worst_fit_AP>>\n");
	}
	if (method_index == GHz_WF_HPF) {
		printf("<<GHz_worst_fit_HPF>>\n");
	}
	if (method_index == GHz_WF_TD) {
		printf("<<GHz_worst_fit_TD>>\n");
	}
	if (method_index == cost_WF_AP) {
		printf("<<cost_worst_fit_AP>>\n");
	}
	if (method_index == cost_WF_HPF) {
		printf("<<cost_worst_fit_HPF>>\n");
	}
	if (method_index == cost_WF_TD) {
		printf("<<cost_worst_fit_TD>>\n");
	}
	if (method_index == TA_CR_AP) {
		printf("<<TA-CR_AP>>\n");
	}
	if (method_index == TA_CR_HPF) {
		printf("<<TA-CR_HPF>>\n");
	}
	
	double total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[_selected_set[ch]];
	}

	double total_cost = 0; 
	double remained_GHz[NUM_OF_ES + 1]; // processing capacity[es] - used_GHz[es] �ϸ� remained_GHz[es] �ϸ� ����. ��� ����� ���� GHz ����� ����.

	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
		total_cost += cost;
		remained_GHz[ES] = _server_list[ES].processing_capacity - _used_GHz[ES];
	}
	std::printf(" total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n\n", total_GHz, total_pwq, total_cost);
}

void GHz_worst_fit_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - �� ES server�� coverage�� Ȯ���ϰ�, ����� GHz�� ���� ���� ES�� �Ҵ��Ѵ�. 
	//���� ���� - ���� �α⵵�� ���� ä���� �켱������ �����Ͽ� ��� version�� Ʈ�����ڵ��ϰ�, �� version�� ���� ES�� (������ ������ ��) �Ҵ��Ѵ�.

	bool** is_allocated_for_versions = (bool**)malloc(sizeof(bool*) * (CHANNEL_NUM + 1));
	set<pair<double, int>, greater<pair<double, int>> > channel_popularities_set;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		is_allocated_for_versions[ch] = (bool*)malloc(sizeof(bool) * (_version_set->version_num));
		channel_popularities_set.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));

		for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
			is_allocated_for_versions[ch][ver] = false;
		}
	}

	while (!channel_popularities_set.empty()) {
		int ch = (*channel_popularities_set.begin()).second;
		channel_popularities_set.erase(channel_popularities_set.begin()); //���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
		set<pair<double, int>> lowest_used_GHz_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES])
				lowest_used_GHz_of_ES.insert(make_pair(_server_list[ES].processing_capacity - _used_GHz[ES], ES));
		}

		while (!lowest_used_GHz_of_ES.empty()) {
			int ES = (*lowest_used_GHz_of_ES.begin()).second;
			lowest_used_GHz_of_ES.erase(lowest_used_GHz_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[1], _model);
				else {
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[1] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = ES;
				_ES_count[ES]++;

				is_allocated_for_versions[ch][1] = true;
				break;
			} //������ �� ���� ��� �Ҵ�.
		}

		if (!is_allocated_for_versions[ch][1]) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][ver]++;

				is_allocated_for_versions[ch][1] = true;
			}
		}
	}
	int alloc_cnt = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		channel_popularities_set.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
		if (is_allocated_for_versions[ch][1]) {
			alloc_cnt++;
		}
	}
	if (alloc_cnt == CHANNEL_NUM) {
		while (!channel_popularities_set.empty()) {
			int ch = (*channel_popularities_set.begin()).second;
			channel_popularities_set.erase(channel_popularities_set.begin());
			//2~version_num-1����
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				is_allocated_for_versions[ch][ver] = false;
				//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
				set<pair<double, int>> lowest_used_GHz_of_ES;
				for (int ES = 1; ES <= NUM_OF_ES; ES++) {
					if (_channel_list[ch].available_server_list[ES])
						lowest_used_GHz_of_ES.insert(make_pair(_server_list[ES].processing_capacity - _used_GHz[ES], ES));
				}

				while (!lowest_used_GHz_of_ES.empty()) {
					int ES = (*lowest_used_GHz_of_ES.begin()).second;
					lowest_used_GHz_of_ES.erase(lowest_used_GHz_of_ES.begin());

					double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
					double total_cost = 0;
					for (int es = 1; es <= NUM_OF_ES; es++) {
						if (es == ES)
							total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _model);
						else {
							//if (get_ES_total_count(es, _version_set))
							total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
						}
					}

					if ((GHz - _channel_list[ch].video_GHz[ver] >= 0) && (total_cost <= _cost_limit)) {
						_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

						_selected_ES[ch][ver] = ES;
						_ES_count[ES]++;
						//ES_version_count_in_comparison_schemes[ES][ver]++;

						is_allocated_for_versions[ch][ver] = true;
						break;
					} //������ �� ���� ��� �Ҵ�.
					//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
				}

				if (!is_allocated_for_versions[ch][ver]) { //��� ������ �Ҵ��� �Ұ����� ������
					double GHz = _server_list[0].processing_capacity - _used_GHz[0];
					if (GHz - _channel_list[ch].video_GHz[ver] >= 0) {
						_used_GHz[0] += _channel_list[ch].video_GHz[ver];

						_selected_ES[ch][ver] = 0;
						_ES_count[0]++;
						//ES_version_count_in_comparison_schemes[0][ver]++;

						is_allocated_for_versions[ch][ver] = true;
					}
				}
			}

			bool is_feasible = true;
			int cnt = 0;
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				if (is_allocated_for_versions[ch][ver])
					cnt++;
			}
			if (cnt < _version_set->version_num - 2) // 1�̶� ���� ���� -2
				is_feasible = false;

			if (!is_feasible) { //������� ������ ��. ������ �Ҵ��� �� ���� Ǯ��.
				for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
					if (is_allocated_for_versions[ch][ver]) {
						int ES = _selected_ES[ch][ver];
						_selected_ES[ch][ver] = -1;
						_ES_count[ES]--;
						if (_ES_count[ES])
							_used_GHz[ES] -= _channel_list[ch].video_GHz[ver];
						else
							_used_GHz[ES] = 0;
					}
				}
			}
		}
	}
}

void GHz_worst_fit_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - �� ES server�� coverage�� Ȯ���ϰ�, ����� GHz�� ���� ���� ES�� �Ҵ��Ѵ�. 
	//���� ���� - ���� �α⵵�� ���� ä��-������ �켱������ �����Ͽ� ES�� (������ ������ ��) �Ҵ��Ѵ�.

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > version_popularities_set;
	
	//ó���� 1�� ������ ���ؼ��� set�� �����Ѵ�.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		version_popularities_set.insert(make_pair(_channel_list[ch].popularity[1], make_pair(ch, 1)));
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		//���� ��� ä���� 1�� ������ �Ҵ��Ѵ�.
		int ch = (*version_popularities_set.begin()).second.first;
		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
		set<pair<double, int>> lowest_used_GHz_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if(_channel_list[ch].available_server_list[ES])
				lowest_used_GHz_of_ES.insert(make_pair(_server_list[ES].processing_capacity - _used_GHz[ES], ES));
		}

		while (!lowest_used_GHz_of_ES.empty()) {
			int ES = (*lowest_used_GHz_of_ES.begin()).second;
			lowest_used_GHz_of_ES.erase(lowest_used_GHz_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[1], _model);
				else{
					//if (get_ES_total_count(es, _version_set))
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[1] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][1]++;

				is_allocated = true;
				break;
			} //������ �� ���� ��� �Ҵ�.
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][1], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][1]++;
			}
		}
	}

	//��� ä���� 2~N^ver-1 �����鿡 ���� �Ҵ��� �����Ѵ�.
	version_popularities_set.clear();
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			version_popularities_set.insert(make_pair(_channel_list[ch].popularity[ver], make_pair(ch, ver)));
		}
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		int ch = (*version_popularities_set.begin()).second.first;
		int ver = (*version_popularities_set.begin()).second.second;

		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, �� ES�� �Ҵ�� ver�� ������ ������ ���� �������� ����. 
		set<pair<int, int>> lowest_used_GHz_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES])
				lowest_used_GHz_of_ES.insert(make_pair(_server_list[ES].processing_capacity - _used_GHz[ES], ES));
		}

		while (!lowest_used_GHz_of_ES.empty()) {
			int ES = (*lowest_used_GHz_of_ES.begin()).second;
			lowest_used_GHz_of_ES.erase(lowest_used_GHz_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[ver] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][ver]++;

				is_allocated = true;
				break;
			}
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[ver] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][ver]++;

				break;
			}
		}
	}
}


void GHz_worst_fit_TD_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - �� ES server�� coverage�� Ȯ���ϰ�, ����� GHz�� ���� ���� ES�� �Ҵ��Ѵ�. 
	//���� ���� - TD

	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}
	TD_phase(_server_list, _channel_list, _version_set, GHz_limit, _selected_set);

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > version_popularities_set;

	//ó���� 1�� ������ ���ؼ��� set�� �����Ѵ�.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		version_popularities_set.insert(make_pair(_channel_list[ch].popularity[1], make_pair(ch, 1)));
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		//���� ��� ä���� 1�� ������ �Ҵ��Ѵ�.
		int ch = (*version_popularities_set.begin()).second.first;
		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
		set<pair<double, int>> lowest_used_GHz_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES])
				lowest_used_GHz_of_ES.insert(make_pair(_server_list[ES].processing_capacity - _used_GHz[ES], ES));
		}

		while (!lowest_used_GHz_of_ES.empty()) {
			int ES = (*lowest_used_GHz_of_ES.begin()).second;
			lowest_used_GHz_of_ES.erase(lowest_used_GHz_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[1], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[1] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][1]++;

				is_allocated = true;
				break;
			} //������ �� ���� ��� �Ҵ�.
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][1], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][1]++;
			}
		}
	}

	//��� ä���� 2~N^ver-1 �����鿡 ���� �Ҵ��� �����Ѵ�.
	version_popularities_set.clear();
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = _selected_set[ch];
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // ������ ������ set���� �Ҵ��ߴ� GHz�� ���� ������ �ش�. 
				version_popularities_set.insert(make_pair(_channel_list[ch].popularity[ver], make_pair(ch, ver)));
			}
		}
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		int ch = (*version_popularities_set.begin()).second.first;
		int ver = (*version_popularities_set.begin()).second.second;

		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, �� ES�� �Ҵ�� ver�� ������ ������ ���� �������� ����. 
		set<pair<int, int>> lowest_used_GHz_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES])
				lowest_used_GHz_of_ES.insert(make_pair(_server_list[ES].processing_capacity - _used_GHz[ES], ES));
		}

		while (!lowest_used_GHz_of_ES.empty()) {
			int ES = (*lowest_used_GHz_of_ES.begin()).second;
			lowest_used_GHz_of_ES.erase(lowest_used_GHz_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[ver] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][ver]++;

				is_allocated = true;
				break;
			}
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[ver] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][ver]++;

				break;
			}
		}
	}
}

void cost_worst_fit_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - �� ES server�� coverage�� Ȯ���ϰ�, ����� cost�� ���� ���� ES�� �Ҵ��Ѵ�. 
	//���� ���� - ���� �α⵵�� ���� ä���� �켱������ �����Ͽ� ��� version�� Ʈ�����ڵ��ϰ�, �� version�� ���� ES�� (������ ������ ��) �Ҵ��Ѵ�.

	bool** is_allocated_for_versions = (bool**)malloc(sizeof(bool*) * (CHANNEL_NUM + 1));
	set<pair<double, int>, greater<pair<double, int>> > channel_popularities_set;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		is_allocated_for_versions[ch] = (bool*)malloc(sizeof(bool) * (_version_set->version_num));
		channel_popularities_set.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));

		for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
			is_allocated_for_versions[ch][ver] = false;
		}
	}

	while (!channel_popularities_set.empty()) {
		int ch = (*channel_popularities_set.begin()).second;
		channel_popularities_set.erase(channel_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
		set<pair<double, int>> lowest_cost_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES]) {
				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[1], _model);
				lowest_cost_of_ES.insert(make_pair(cost, ES));
			}
		}

		while (!lowest_cost_of_ES.empty()) {
			int ES = (*lowest_cost_of_ES.begin()).second;
			lowest_cost_of_ES.erase(lowest_cost_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[1], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[1] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][ver]++;

				is_allocated_for_versions[ch][1] = true;
				break;
			} //������ �� ���� ��� �Ҵ�.
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
		}

		if (!is_allocated_for_versions[ch][1]) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][ver]++;

				is_allocated_for_versions[ch][1] = true;
			}
		}
	}

	int alloc_cnt = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		channel_popularities_set.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
		if (is_allocated_for_versions[ch][1]) {
			alloc_cnt++;
		}
	}
	if (alloc_cnt == CHANNEL_NUM) {
		while (!channel_popularities_set.empty()) {
			int ch = (*channel_popularities_set.begin()).second;
			channel_popularities_set.erase(channel_popularities_set.begin());
			//2~version_num-1����
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				is_allocated_for_versions[ch][ver] = false;
				//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
				set<pair<double, int>> lowest_cost_of_ES;
				for (int ES = 1; ES <= NUM_OF_ES; ES++) {
					if (_channel_list[ch].available_server_list[ES]) {
						double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _model);
						lowest_cost_of_ES.insert(make_pair(cost, ES));
					}
				}

				while (!lowest_cost_of_ES.empty()) {
					int ES = (*lowest_cost_of_ES.begin()).second;
					lowest_cost_of_ES.erase(lowest_cost_of_ES.begin());

					double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
					double total_cost = 0;
					for (int es = 1; es <= NUM_OF_ES; es++) {
						if (es == ES)
							total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _model);
						else {
							//if (get_ES_total_count(es, _version_set))
							total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
						}
					}

					if ((GHz - _channel_list[ch].video_GHz[ver] >= 0) && (total_cost <= _cost_limit)) {
						_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

						_selected_ES[ch][ver] = ES;
						_ES_count[ES]++;
						//ES_version_count_in_comparison_schemes[ES][ver]++;

						is_allocated_for_versions[ch][ver] = true;
						break;
					} //������ �� ���� ��� �Ҵ�.
					//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
				}

				if (!is_allocated_for_versions[ch][ver]) { //��� ������ �Ҵ��� �Ұ����� ������
					double GHz = _server_list[0].processing_capacity - _used_GHz[0];
					if (GHz - _channel_list[ch].video_GHz[ver] >= 0) {
						_used_GHz[0] += _channel_list[ch].video_GHz[ver];

						_selected_ES[ch][ver] = 0;
						_ES_count[0]++;
						//ES_version_count_in_comparison_schemes[0][ver]++;

						is_allocated_for_versions[ch][ver] = true;
					}
				}
			}

			bool is_feasible = true;
			int cnt = 0;
			for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
				if (is_allocated_for_versions[ch][ver])
					cnt++;
			}
			if (cnt < _version_set->version_num - 2) // 1�̶� ���� ���� -2
				is_feasible = false;

			if (!is_feasible) { //������� ������ ��. ������ �Ҵ��� �� ���� Ǯ��.
				for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
					if (is_allocated_for_versions[ch][ver]) {
						int ES = _selected_ES[ch][ver];
						_selected_ES[ch][ver] = -1;
						_ES_count[ES]--;
						if (_ES_count[ES])
							_used_GHz[ES] -= _channel_list[ch].video_GHz[ver];
						else
							_used_GHz[ES] = 0;
					}
				}
			}
		}
	}
}

void cost_worst_fit_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - �� ES server�� coverage�� Ȯ���ϰ�, ����� cost�� ���� ���� ES�� �Ҵ��Ѵ�. 
	//���� ���� - ���� �α⵵�� ���� ä��-������ �켱������ �����Ͽ� ES�� (������ ������ ��) �Ҵ��Ѵ�.

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > version_popularities_set;

	//ó���� 1�� ������ ���ؼ��� set�� �����Ѵ�.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		version_popularities_set.insert(make_pair(_channel_list[ch].popularity[1], make_pair(ch, 1)));
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		//���� ��� ä���� 1�� ������ �Ҵ��Ѵ�.
		int ch = (*version_popularities_set.begin()).second.first;
		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
		set<pair<double, int>> lowest_cost_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES]) {
				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[1], _model);
				lowest_cost_of_ES.insert(make_pair(cost, ES));
			}
		}

		while (!lowest_cost_of_ES.empty()) {
			int ES = (*lowest_cost_of_ES.begin()).second;
			lowest_cost_of_ES.erase(lowest_cost_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[1], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[1] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][1]++;

				is_allocated = true;
				break;
			} //������ �� ���� ��� �Ҵ�.
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][1], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][1]++;
			}
		}
	}

	//��� ä���� 2~N^ver-1 �����鿡 ���� �Ҵ��� �����Ѵ�.
	version_popularities_set.clear();
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			version_popularities_set.insert(make_pair(_channel_list[ch].popularity[ver], make_pair(ch, ver)));
		}
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		int ch = (*version_popularities_set.begin()).second.first;
		int ver = (*version_popularities_set.begin()).second.second;

		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, �� ES�� �Ҵ�� ver�� ������ ������ ���� �������� ����. 
		set<pair<int, int>> lowest_cost_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES]) {
				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[1], _model);
				lowest_cost_of_ES.insert(make_pair(cost, ES));
			}
		}

		while (!lowest_cost_of_ES.empty()) {
			int ES = (*lowest_cost_of_ES.begin()).second;
			lowest_cost_of_ES.erase(lowest_cost_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
						total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[ver] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][ver]++;

				is_allocated = true;
				break;
			}
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[ver] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][ver]++;

				break;
			}
		}
	}
}

void cost_worst_fit_TD_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - �� ES server�� coverage�� Ȯ���ϰ�, ����� cost�� ���� ���� ES�� �Ҵ��Ѵ�. 
	//���� ���� - TD

	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}
	TD_phase(_server_list, _channel_list, _version_set, GHz_limit, _selected_set);

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > version_popularities_set;

	//ó���� 1�� ������ ���ؼ��� set�� �����Ѵ�.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		version_popularities_set.insert(make_pair(_channel_list[ch].popularity[1], make_pair(ch, 1)));
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		//���� ��� ä���� 1�� ������ �Ҵ��Ѵ�.
		int ch = (*version_popularities_set.begin()).second.first;
		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
		set<pair<double, int>> lowest_cost_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES]) {
				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[1], _model);
				lowest_cost_of_ES.insert(make_pair(cost, ES));
			}
		}

		while (!lowest_cost_of_ES.empty()) {
			int ES = (*lowest_cost_of_ES.begin()).second;
			lowest_cost_of_ES.erase(lowest_cost_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[1], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[1] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][1]++;

				is_allocated = true;
				break;
			} //������ �� ���� ��� �Ҵ�.
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][1], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[1];

				_selected_ES[ch][1] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][1]++;
			}
		}
	}

	//��� ä���� 2~N^ver-1 �����鿡 ���� �Ҵ��� �����Ѵ�.
	version_popularities_set.clear();
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = _selected_set[ch];
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // ������ ������ set���� �Ҵ��ߴ� GHz�� ���� ������ �ش�. 
				version_popularities_set.insert(make_pair(_channel_list[ch].popularity[ver], make_pair(ch, ver)));
			}
		}
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		int ch = (*version_popularities_set.begin()).second.first;
		int ver = (*version_popularities_set.begin()).second.second;

		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, �� ES�� �Ҵ�� ver�� ������ ������ ���� �������� ����. 
		set<pair<int, int>> lowest_cost_of_ES;
		for (int ES = 1; ES <= NUM_OF_ES; ES++) {
			if (_channel_list[ch].available_server_list[ES]) {
				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[1], _model);
				lowest_cost_of_ES.insert(make_pair(cost, ES));
			}
		}

		while (!lowest_cost_of_ES.empty()) {
			int ES = (*lowest_cost_of_ES.begin()).second;
			lowest_cost_of_ES.erase(lowest_cost_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - _used_GHz[ES];
			double total_cost = 0;
			for (int es = 1; es <= NUM_OF_ES; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es] + _channel_list[ch].video_GHz[ver], _model);
				else {
					//if (get_ES_total_count(es, _version_set))
					total_cost += calculate_ES_cost(&(_server_list[es]), _used_GHz[es], _model);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[ver] >= 0) && (total_cost <= _cost_limit)) {
				_used_GHz[ES] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = ES;
				_ES_count[ES]++;
				//ES_version_count_in_comparison_schemes[ES][ver]++;

				is_allocated = true;
				break;
			}
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - _used_GHz[0];
			if (GHz - _channel_list[ch].video_GHz[ver] >= 0) {
				_used_GHz[0] += _channel_list[ch].video_GHz[ver];

				_selected_ES[ch][ver] = 0;
				_ES_count[0]++;
				//ES_version_count_in_comparison_schemes[0][ver]++;

				break;
			}
		}
	}
}

void TA_CR_phase_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - TA+CR phase
	//���� ���� - ���� �α⵵�� ���� ä���� �켱������ �����Ͽ� ��� version�� Ʈ�����ڵ��ϰ�, �� version�� ���� ES�� (������ ������ ��) �Ҵ��Ѵ�.
	
	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}
	
	double total_GHz = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//selected_ES[ch] = 0;
		_selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}

	set<pair<double, int>> channel_popularities_set;
	//_version_set->version_set_num(N^set)���� �ʱ�ȭ�� ���¿��� set�� ����.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		channel_popularities_set.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	while (channel_popularities_set.size()) {
		int ch = (*channel_popularities_set.begin()).second;
		channel_popularities_set.erase(channel_popularities_set.begin());//�� �� ������

		double expected_total_GHz = total_GHz - _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num] + _channel_list[ch].sum_of_version_set_GHz[1];
		
		if (expected_total_GHz < GHz_limit) {
			break;
		}

		total_GHz = expected_total_GHz;
		_selected_set[ch] = 1;
	}

	TA_phase(_server_list, _channel_list, _version_set, _cost_limit, _selected_set, _selected_ES, _used_GHz, _ES_count, _model);
	
	double total_cost = 0;
	//double remained_GHz[ES_NUM + 1]; // processing capacity[es] - _used_GHz[es] �ϸ� remained_GHz[es] �ϸ� ����. ��� ����� ���� GHz ����� ����.
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		total_cost += calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
		//remained_GHz[ES] = _server_list[ES].processing_capacity - _used_GHz[ES];
	}

	CR_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, _selected_set, _selected_ES, _used_GHz, _ES_count, _model);
}


void TA_CR_phase_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	//���� ���� - TA+CR phase
	//���� ���� - ���� �α⵵�� ���� ä��-������ �켱������ �����Ͽ� ES�� (������ ������ ��) �Ҵ��Ѵ�.

	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}

	double total_GHz = 0;
	
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>>> version_popularities_set;
	//_version_set->version_set_num(N^set)���� �ʱ�ȭ�� ���¿��� set�� ����.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		version_popularities_set.insert(make_pair(_channel_list[ch].popularity[1], make_pair(ch, 1)));
	}

	while (version_popularities_set.size()) {
		int ch = (*version_popularities_set.begin()).second.first;
		//int ver = (*version_popularities_set.begin()).second.second;

		version_popularities_set.erase(version_popularities_set.begin());//�� �� ������
		double expected_total_GHz = total_GHz + _channel_list[ch].video_GHz[1];
		
		if (expected_total_GHz > GHz_limit) {
			break;
		}

		total_GHz = expected_total_GHz;
		//_selected_set[ch] = 1;
	}

	version_popularities_set.clear();
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) { //�ҽ��� ���� 1080p
			version_popularities_set.insert(make_pair(_channel_list[ch].popularity[ver], make_pair(ch, ver)));
		}
	}

	while (version_popularities_set.size()) {
		int ch = (*version_popularities_set.begin()).second.first;
		int ver = (*version_popularities_set.begin()).second.second;

		version_popularities_set.erase(version_popularities_set.begin());//�� �� ������
		double expected_total_GHz = total_GHz + _channel_list[ch].video_GHz[ver];
		
		if (expected_total_GHz > GHz_limit) {
			break;
		}

		total_GHz = expected_total_GHz;
		_selected_set[ch] += (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
	}

	TA_phase(_server_list, _channel_list, _version_set, _cost_limit, _selected_set, _selected_ES, _used_GHz, _ES_count, _model);

	double total_cost = 0;
	//double remained_GHz[ES_NUM + 1]; // processing capacity[es] - _used_GHz[es] �ϸ� remained_GHz[es] �ϸ� ����. ��� ����� ���� GHz ����� ����.
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		total_cost += calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
		//remained_GHz[ES] = _server_list[ES].processing_capacity - _used_GHz[ES];
	}

	CR_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, _selected_set, _selected_ES, _used_GHz, _ES_count, _model);
}


/*short get_ES_total_count(int ES, bitrate_version_set* _version_set) {
	short ES_total_count = 0;
	for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
		ES_total_count += ES_version_count_in_comparison_schemes[ES][ver];
	}
	return ES_total_count;
}*/