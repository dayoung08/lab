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
	}

	double used_GHz[NUM_OF_ES + 1];
	short ES_count[NUM_OF_ES + 1];
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		used_GHz[ES] = 0;
	}
	memset(ES_count, 0, (sizeof(short) * (NUM_OF_ES + 1)));

	double first_GHz = 0; //lowest version�� Ʈ�����ڵ��Ҷ�
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
	}
	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		GHz_limit += _server_list[ES].processing_capacity;
	}
	printf("lowest version�� Ʈ�����ڵ� ���� �� %lf GHz / GHz �� �� %lf GHz\n\n", first_GHz, GHz_limit);
	if (GHz_limit < first_GHz) {
		printf("GHz�� ���ڶ� ��Ȳ/Channel ���� ���̰ų�, ���� ���� �ø� ��\n");
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
	double remained_GHz[NUM_OF_ES + 1]; // processing capacity[es] - _used_GHz[es] �ϸ� remained_GHz[es] �ϸ� ����. ��� ����� ���� GHz ����� ����.
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
	//���߿� �� ������� �Լ� ������ ��. �׷��� ���� �� ���ϴ�.
	//1. TD phase
	double total_GHz = 0;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		//selected_ES[ch] = 0;
		_selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}
	set<pair<double, pair<int, int>>> list_TD;
	//_version_set->version_set_num(N^set)���� �ʱ�ȭ�� ���¿��� set�� ����.
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		for (int set = 1; set <= _version_set->version_set_num - 1; set++) { //�ҽ��� ���� 1080p
			double slope = (_channel_list[ch].sum_of_pwq[_version_set->version_set_num] - _channel_list[ch].sum_of_pwq[set]) / (_channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num] - _channel_list[ch].sum_of_version_set_GHz[set]);
			list_TD.insert(make_pair(slope, make_pair(ch, set)));
		}
	}

	while (list_TD.size()) {
		int ch = (*list_TD.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int set = (*list_TD.begin()).second.second; //slope�� ���� ū ���� � ��Ʈ�ΰ�?

		list_TD.erase(list_TD.begin());//�� �� ������
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
	//210721 �� �κи� �������� ��� GHz�� �����ɷ� �ߴµ�, cost�� ������. �׸��� �� ���� ����� ����.
	set<pair<double, int>> remained_GHz_of_ESs_set;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		remained_GHz_of_ESs_set.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
	}

	double** slopes_of_list_TA;
	slopes_of_list_TA = (double**)malloc(sizeof(double*) * (NUM_OF_CHANNEL + 1));
	for (int row = 1; row <= NUM_OF_CHANNEL; row++) {
		slopes_of_list_TA[row] = (double*)malloc(sizeof(double) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			slopes_of_list_TA[row][col] = -1;
		}
	}
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > list_TA;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		double slope = _channel_list[ch].pwq[1] / _channel_list[ch].video_GHz[1];

		//�̰� pwq/GHz�� pwq/cost�� linear �𵨿��� �Ȱ���, onoff model���� ���ʿ� cost�� Ʋ���Ű� ��굵 �ȵ�
		list_TA.insert(make_pair(slope, make_pair(ch, 1)));
		slopes_of_list_TA[ch][1] = slope;
	}

	double total_cost = 0;
	while (list_TA.size()) { // ��� ä���� 1���� �Ҵ��ؾ��ϹǷ�
		int ch = (*list_TA.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		list_TA.erase(list_TA.begin());//�� �� ������

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

			ES = (*pos).second; // ���� ���� GHz�� ���� ������ �����ΰ�?

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
	//1�� �Ҵ� �Ϸ�

	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		int set = _selected_set[ch];
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // ������ ������ set���� �Ҵ��ߴ� GHz�� ���� ������ �ش�. 
				//double slope = _channel_list[ch].pwq[ver] / _channel_list[ch].video_GHz[ver];
				int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
				double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];

				//�̰� pwq/GHz�� pwq/cost�� linear �𵨿��� �Ȱ���, onoff model���� ���ʿ� cost�� Ʋ���Ű� ��굵 �ȵ�
				list_TA.insert(make_pair(slope, make_pair(ch, ver)));
				slopes_of_list_TA[ch][ver] = slope;
			}
		}
	}
	while (list_TA.size()) {
		int ch = (*list_TA.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int ver = (*list_TA.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
		list_TA.erase(list_TA.begin());//�� �� ������

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

			ES = (*pos).second; // ���� ���� GHz�� ���� ������ �����ΰ�?
			GHz = (*pos).first; // �� ������ GHz�� ���ΰ�?

			if ((_channel_list[ch].available_server_list[ES]) && (_used_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity)) {
				//210715 ���� ���� ã��
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

		//cost limit�� ������ �� ���� ES ���� �� �������� �����ϰ�,
		//���ŵ� �������� CTS�� �ű����, CTS capacity �Ѵ� ���� �����Ѵ�. // 20210713 �߰���.
		if (_model == CPU_USAGE_MODEL) {
			set<pair<double, pair<int, int>>> list_CR;
			// slope (pwq/cost) �� / channel-version
			// pwq �� / channel-version
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == 0) { // 20210713 ������.
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
					//������� ����
				}
			}

			while (list_CR.size()) {
				int ch_in_ES = (*list_CR.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
				int ver_in_ES = (*list_CR.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
				//double slope = (*list_CR.begin()).first;
				list_CR.erase(list_CR.begin());// list_CR�� �� �� ������

				double prev_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);

				if (ver_in_ES > 1) {
					int set_temp = _selected_set[ch_in_ES] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver_in_ES - 1)));
					double slope = (_channel_list[ch_in_ES].sum_of_pwq[_selected_set[ch_in_ES]] - _channel_list[ch_in_ES].sum_of_pwq[set_temp]) / _channel_list[ch_in_ES].video_GHz[ver_in_ES];
					versions_in_CTS.insert(make_pair(slope, make_pair(ch_in_ES, ver_in_ES))); //CTS�� �ӽ� �Ҵ�
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
			// pwq �� / channel-version
			for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == 0) { // 20210713 ������.
						int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
						double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
						versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver)));
					}
					else if (_selected_ES[ch][ver] >= 1) { // ES�� �Ҵ�� �������� pwq�� ���� ������
						pwq[_selected_ES[ch][ver]] += _channel_list[ch].pwq[ver];
					}
				}
			}

			set<pair<double, int>> list_CR;
			// slope (pwq/cost) �� / ES
			for (int ES = 1; ES <= NUM_OF_ES; ES++) {
				double slope = pwq[ES] / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				list_CR.insert(make_pair(slope, ES));
			}

			int cnt = 0;
			while (list_CR.size()) {
				cnt++;
				int ES = (*list_CR.begin()).second; // slope�� ���� ū ���� � �����ΰ�?
				list_CR.erase(list_CR.begin());//�� �� ������

				double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
				_ES_count[0] += _ES_count[ES];
				_ES_count[ES] = 0;
				_used_GHz[0] += _used_GHz[ES];
				_used_GHz[ES] = 0;
				_total_cost -= cost;
				//��������� cost ������ ES�� �Ҵ�� version���� ���� ��.

				for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
					for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
						if (_selected_ES[ch][ver] == ES) {
							//double slope = _channel_list[ch].pwq[ver] / _channel_list[ch].video_GHz[ver];
							int set_temp = _selected_set[ch] - (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)));
							double slope = (_channel_list[ch].sum_of_pwq[_selected_set[ch]] - _channel_list[ch].sum_of_pwq[set_temp]) / _channel_list[ch].video_GHz[ver];
							versions_in_CTS.insert(make_pair(slope, make_pair(ch, ver))); //CTS�� �ӽ� �Ҵ�
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

			_ES_count[0]--;
			_used_GHz[0] -= _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];
			_selected_ES[ch_in_CTS][ver_in_CTS] = -1;

			if (_used_GHz[0] <= _server_list[0].processing_capacity) {
				break;
			}
		}
	}
	//set ����ϱ�
	set_version_set(_version_set, _selected_set, _selected_ES);
}