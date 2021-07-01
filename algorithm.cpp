
#include "head.h"

//�˰��� ���⼭���� ���� ¥����

//double _used_GHz[ES_NUM + 1];
//double total_transfer_data_size[ES_NUM + 1];//�ǽð����� �����ϴ� ������ �������� �� ����� ����
//short _ES_count[ES_NUM + 1];

void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, int _model) {
	short selected_set[CHANNEL_NUM + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
	//short selected_ES[CHANNEL_NUM + 1];// �� ä���� � es���� �Ҵ�Ǿ��°�.
	//�������� ������ Ʈ�����ڵ� ���ؼ� �迭 ũ�Ⱑ ������.
	short** selected_ES;

	selected_ES = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int row = 1; row <= CHANNEL_NUM; row++) {
		selected_ES[row] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int col = 1; col <= _version_set->version_num - 1; col++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			selected_ES[row][col] = -1;
		}
	}

	double _used_GHz[ES_NUM + 1];
	//double total_transfer_data_size[ES_NUM + 1];//�ǽð����� �����ϴ� ������ �������� �� ����� ����
	short _ES_count[ES_NUM + 1];
	memset(_ES_count, 0, (sizeof(short) * (ES_NUM + 1)));
	for (int ES = 0; ES <= ES_NUM; ES++) {
		_used_GHz[ES] = 0;
		//total_transfer_data_size[ES] = 0;
	}

	double first_GHz = 0; //lowest version�� Ʈ�����ڵ��Ҷ�
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
	}
	double GHz_limit = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= ES_NUM; ES++) {
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
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	std::printf("=TD= total_GHz : %lf GHz, total_pwq : %lf\n", total_GHz, total_pwq);

	
	TA_phase(_server_list, _channel_list, _version_set, _cost_limit, selected_set, selected_ES, _used_GHz, _ES_count, _model);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	double total_cost = 0;
	double remained_GHz[ES_NUM + 1]; // processing capacity[es] - _used_GHz[es] �ϸ� remained_GHz[es] �ϸ� ����. ��� ����� ���� GHz ����� ����.
	for (int ES = 0; ES <= ES_NUM; ES++) {
		total_cost += calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
		remained_GHz[ES] = _server_list[ES].processing_capacity - _used_GHz[ES];
	}
	std::printf("=TA= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);

	if (_model == CPU_USAGE_MODEL) {
		CR_usage_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, selected_set, selected_ES, _used_GHz, _ES_count, _model);
	}
	if (_model == LEASING_MODEL) {
		CR_leasing_phase(_server_list, _channel_list, _version_set, total_cost, _cost_limit, selected_set, selected_ES, _used_GHz, _ES_count, _model);
	}
	total_GHz = 0;
	total_pwq = 0;
	int cnt = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];

		if (selected_ES[ch][1] != -1)
			cnt++;
	}

	total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		total_cost += calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
		remained_GHz[ES] = _server_list[ES].processing_capacity - _used_GHz[ES];
	}


	std::printf("=CR= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf $\n", total_GHz, total_pwq, total_cost);
}

void TD_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _GHz_limit, short* _selected_set) {
	//���߿� �� ������� �Լ� ������ ��. �׷��� ���� �� ���ϴ�.
	//1. TD phase
	double total_GHz = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//selected_ES[ch] = 0;
		_selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}
	set<pair<double, pair<int, int>>> list_TD;
	//_version_set->version_set_num(N^set)���� �ʱ�ȭ�� ���¿��� set�� ����.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int set = 1; set <= _version_set->version_set_num - 1; set++) { //�ҽ��� ���� 1080p
			double slope = (_channel_list[ch].sum_of_pwq[_version_set->version_set_num] - _channel_list[ch].sum_of_pwq[set]) / (_channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num] - _channel_list[ch].sum_of_version_set_GHz[set]);
			list_TD.insert(make_pair(slope, make_pair(ch, set)));
		}
	}

	while (list_TD.size()) {
		int ch = (*list_TD.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int set = (*list_TD.begin()).second.second; //slope�� ���� ū ���� � ��Ʈ�ΰ�?

		list_TD.erase(list_TD.begin());//�� �� ������
		//int prev_����_node = selected_BN[channel];
		int prev_set = _selected_set[ch];
		if (_channel_list[ch].sum_of_version_set_GHz[set] < _channel_list[ch].sum_of_version_set_GHz[prev_set]) {
			double expected_total_GHz = total_GHz - _channel_list[ch].sum_of_version_set_GHz[prev_set] + _channel_list[ch].sum_of_version_set_GHz[set];
			total_GHz = expected_total_GHz;
			if (expected_total_GHz < _GHz_limit) {
				break;
			}
			_selected_set[ch] = set; //210615 ���� ����
		}
	}
}

void TA_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	// 2-1. TA phase
	set<pair<double, int>> remained_GHz_of_ESs_set;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		remained_GHz_of_ESs_set.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
	}

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > list_TA;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		double slope = _channel_list[ch].pwq[1] / _channel_list[ch].video_GHz[1];
		//double slope = _channel_list[ch].pwq[1] / calculate_ES_cost(&(_server_list[_selected_ES[ch][1]]), _channel_list[ch].video_GHz[1], _model);
		list_TA.insert(make_pair(slope, make_pair(ch, 1)));
	}

	double total_cost = 0;
	while (list_TA.size()) { // ��� ä���� 1���� �Ҵ��ؾ��ϹǷ�
		int ch = (*list_TA.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		list_TA.erase(list_TA.begin());//�� �� ������

		set <pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.end();

		int ES = -1;
		double GHz = 0;
		double prev_cost = 0;
		double curr_cost = 0;

		bool is_allocated_CTS = false;
		while (true) {
			pos--;
			if (pos == remained_GHz_of_ESs_set.begin()) {
				is_allocated_CTS = true;
				break;
			}

			ES = (*pos).second; // ���� ���� GHz�� ���� ������ �����ΰ�?
			GHz = (*pos).first; // �� ������ GHz�� ���ΰ�?

			if ((_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[1] >= 0)) {
				break;
			}
		}

		if (!is_allocated_CTS) {
			/*prev_cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			curr_cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[1], _model);

			if ((total_cost + curr_cost - prev_cost) > _cost_limit) {
				break;
			}
			total_cost += (curr_cost - prev_cost);*/

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
	//1�� �Ҵ� �Ϸ�

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = _selected_set[ch];
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // ������ ������ set���� �Ҵ��ߴ� GHz�� ���� ������ �ش�. 
				double slope = _channel_list[ch].pwq[ver] / _channel_list[ch].video_GHz[ver];
				//double slope = _channel_list[ch].pwq[ver] / calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver], _model);
				list_TA.insert(make_pair(slope, make_pair(ch, ver)));
			}
		}
	}
	while (list_TA.size()) {
		int ch = (*list_TA.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int ver = (*list_TA.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
		list_TA.erase(list_TA.begin());//�� �� ������

		set <pair<double, int>>::iterator pos = remained_GHz_of_ESs_set.end();
		int ES = -1;
		double GHz = 0;
		double prev_cost = 0;
		double curr_cost = 0;
		bool is_allocated_CTS = false;

		while (true) {
			pos--;
			if (pos == remained_GHz_of_ESs_set.begin()) {
				is_allocated_CTS = true;
				break;
			}

			ES = (*pos).second; // ���� ���� GHz�� ���� ������ �����ΰ�?
			GHz = (*pos).first; // �� ������ GHz�� ���ΰ�?

			if ((_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[ver] >= 0)) {
				break;
			}
		}

		if (!is_allocated_CTS) {
			/*prev_cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			curr_cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES] + _channel_list[ch].video_GHz[ver], _model);

			if ((total_cost + curr_cost - prev_cost) > _cost_limit) {
				break;
			}
			total_cost += (curr_cost - prev_cost);*/

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

	//set ����ϱ�
	set_version_set(_version_set, _selected_set, _selected_ES);
}

// 2-2. CR phase
void CR_usage_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	if (_total_cost > _cost_limit) {
		// �� �� ES���� �� ��, ingesion server�� �ִ� �������� pwq�� ���� ���,
		// (�� CTS�� �Ҵ�� ���� ��, pwq�� ���� ���� ������ ���Ѵ�.)
		// (CTS�� �Ҵ�� ������ ������ �������� �� pwq�� ���� ���, �ش� ������ CTS�� ���� ���� �ű� �ִ� ������ ����.)
		// ES���� �� ���� �ٽ� CTS�� ������, ingesion server���� �� ������ ������ ����.set<pair<double, pair<int, int>>> list_CA_reallocation;
		set<pair<double, pair<int, int>>> list_CR;
		// slope (pwq/cost) �� / channel-version

		set<pair<double, pair<int, int>>> pwq_of_version_in_CTS;
		// pwq �� / channel-version
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			double cost = 0;
			for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
				if (_selected_ES[ch][ver] == 0) { // 0�� CTS�� �Ҵ�� ��
					if (ver > 1) { // CTS�� �Ҵ�� 1�� ������ ���� ���� ����
						pwq_of_version_in_CTS.insert(make_pair(_channel_list[ch].pwq[ver], make_pair(ch, ver)));
					}
				}
				else if (_selected_ES[ch][ver] >= 2) { // ES�� �Ҵ�� ����
					double slope = _channel_list[ch].pwq[ver] / calculate_ES_cost(&(_server_list[_selected_ES[ch][ver]]), _channel_list[ch].video_GHz[ver], _model);
					list_CR.insert(make_pair(slope, make_pair(ch, ver)));
				}
			}
		}
		
		while (list_CR.size()) {
			int ch_in_ES = (*list_CR.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
			int ver_in_ES = (*list_CR.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
			list_CR.erase(list_CR.begin());//�� �� ������

			/*if (_used_GHz[selected_ES[ch][ver]] < 0) {
				cout << "error";
			}*/

			double prev_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			_ES_count[_selected_ES[ch_in_ES][ver_in_ES]]--;

			/*if (!_ES_count[selected_ES[ch][ver]]) {
				_used_GHz[selected_ES[ch][ver]] = 0;
				total_cost -= prev_cost;
			}
			else {*/
			_used_GHz[_selected_ES[ch_in_ES][ver_in_ES]] -= _channel_list[ch_in_ES].video_GHz[ver_in_ES];
			double curr_cost = calculate_ES_cost(&(_server_list[_selected_ES[ch_in_ES][ver_in_ES]]), _used_GHz[_selected_ES[ch_in_ES][ver_in_ES]], _model);
			_total_cost -= (prev_cost - curr_cost);
			//}

			//��������� cost ������ ES���� version ���� ��.
			//���� �� �� version�� CTS�� �Ҵ� �� �� �������� �����Ѵ�.
			//��Ȯ����, CTS�� ���� ���� pwq�� ���� version�� pwq�� ���Ѵ�
			double pwq_in_ES = _channel_list[ch_in_ES].pwq[ver_in_ES];
			double video_GHz_in_ES = _channel_list[ch_in_ES].video_GHz[ver_in_ES];

			int ch_in_CTS = (*pwq_of_version_in_CTS.begin()).second.first;
			int ver_in_CTS = (*pwq_of_version_in_CTS.begin()).second.second;
			double pwq_in_CTS = (*pwq_of_version_in_CTS.begin()).first;
			double video_GHz_in_CTS = _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];

			if ((pwq_in_CTS < pwq_in_ES) &&	((_used_GHz[0] - video_GHz_in_CTS + video_GHz_in_ES) <= _server_list[0].processing_capacity)) {
				_used_GHz[0] -= video_GHz_in_CTS;
				_used_GHz[0] += video_GHz_in_ES;

				pwq_of_version_in_CTS.erase(pwq_of_version_in_CTS.begin());
				pwq_of_version_in_CTS.insert(make_pair(pwq_in_ES, make_pair(ch_in_ES, ver_in_ES)));
				//�������
				_selected_ES[ch_in_ES][ver_in_ES] = 0;
				_selected_ES[ch_in_CTS][ver_in_CTS] = -1;
			}
			else if (ver_in_ES > 1) {
				_selected_ES[ch_in_ES][ver_in_ES] = -1;
			}

			if (_total_cost <= _cost_limit) {
				break;
			}
		}
	}
	//set ����ϱ�
	set_version_set(_version_set, _selected_set, _selected_ES);
}

void CR_leasing_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, short* _ES_count, int _model) {
	if (_total_cost > _cost_limit) {
		// �� �� ES���� �� ��, ingesion server�� �ִ� �������� pwq�� ���� ���,
		// (�� CTS�� �Ҵ�� ���� ��, pwq�� ���� ���� ������ ���Ѵ�.)
		// (CTS�� �Ҵ�� ������ ������ �������� �� pwq�� ���� ���, �ش� ������ CTS�� ���� ���� �ű� �ִ� ������ ����.)
		// ES���� �� ���� �ٽ� CTS�� ������, ingesion server���� �� ������ ������ ����.
		
		double pwq[ES_NUM + 1];
		set<pair<double, pair<int, int>>> pwq_of_version_in_CTS;
		// pwq �� / channel-version
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
				if (_selected_ES[ch][ver] == 0) { // 0�� CTS�� �Ҵ�� ��
					if (ver > 1) { // CTS�� �Ҵ�� 1�� ������ ���� ���� ����
						pwq_of_version_in_CTS.insert(make_pair(_channel_list[ch].pwq[ver], make_pair(ch, ver)));
					}
				}
				else if(_selected_ES[ch][ver] >= 2){ // ES�� �Ҵ�� ����
					pwq[_selected_ES[ch][ver]] += _channel_list[ch].pwq[ver];
				}
			}
		}

		set<pair<double, int>> list_CR;
		// slope (pwq/cost) �� / ES
		for (int ES = 1; ES <= ES_NUM; ES++) {
			double slope = pwq[ES] / calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			list_CR.insert(make_pair(slope, ES));
		}

		bool flag = true;
		while (list_CR.size() && flag) {
			int ES = (*list_CR.begin()).second; // slope�� ���� ū ���� � �����ΰ�?
			list_CR.erase(list_CR.begin());//�� �� ������

			double cost = calculate_ES_cost(&(_server_list[ES]), _used_GHz[ES], _model);
			_ES_count[ES] = 0;
			_used_GHz[ES] = 0;
			_total_cost -= cost;

			//��������� cost ������ ES�� �Ҵ�� version���� ���� ��.
			//���� �� �� version�� CTS�� �Ҵ� �� �� �������� �����Ѵ�.
			//��Ȯ����, CTS�� ���� ���� pwq�� ���� version�� pwq�� ���Ѵ�.

			set<pair<double, pair<int, int>>> pwq_of_version_in_ES;
			// pwq �� / channel-version
			for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
				for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
					if (_selected_ES[ch][ver] == ES) {
						_selected_ES[ch][ver] = -1;//�ϴ� �� ���� ó��

						if (ver == 1) {
							pwq_of_version_in_ES.insert(make_pair(_channel_list[ch].pwq[1], make_pair(ch, 1)));
						}
					}
				}
			}
			// ES�� �ִ� ���� ��, 1�� �������� �������� �ʱ� ����.

			bool is_checked_lowest_version = false;
			while (!pwq_of_version_in_ES.empty()){
				//ES�� ���� �� 1�� ������ �켱 ������.
				int ch_in_ES = (*pwq_of_version_in_ES.begin()).second.first;
				int ver_in_ES = (*pwq_of_version_in_ES.begin()).second.second;
				double pwq_in_ES = _channel_list[ch_in_ES].pwq[ver_in_ES];
				double video_GHz_in_ES = _channel_list[ch_in_ES].video_GHz[ver_in_ES];
				pwq_of_version_in_ES.erase(pwq_of_version_in_ES.begin());

				// CTS ���� �ִ� ������. 2~N^ver�� �������̴�.
				int ch_in_CTS = (*pwq_of_version_in_CTS.begin()).second.first;
				int ver_in_CTS = (*pwq_of_version_in_CTS.begin()).second.second;
				double pwq_in_CTS = _channel_list[ch_in_CTS].pwq[ver_in_CTS];
				double video_GHz_in_CTS = _channel_list[ch_in_CTS].video_GHz[ver_in_CTS];


				if (ver_in_ES == 1 || ((pwq_in_CTS < pwq_in_ES) && ((_used_GHz[0] - video_GHz_in_CTS + video_GHz_in_ES) <= _server_list[0].processing_capacity))){
					_total_cost -= calculate_ES_cost(&(_server_list[0]), _used_GHz[0], _model);

					_used_GHz[0] -= video_GHz_in_CTS;
					_used_GHz[0] += video_GHz_in_ES;

					_total_cost += calculate_ES_cost(&(_server_list[0]), _used_GHz[0], _model);

					pwq_of_version_in_CTS.erase(pwq_of_version_in_CTS.begin());
					pwq_of_version_in_CTS.insert(make_pair(pwq_in_ES, make_pair(ch_in_ES, ver_in_ES)));
					//�������

					_selected_ES[ch_in_ES][ver_in_ES] = 0;
					_selected_ES[ch_in_CTS][ver_in_CTS] = -1;
				}
				/*else {
					_selected_ES[ch_in_ES][ver_in_ES] = -1;
				}*/ // ������ ���� ó���� pwq_of_version_in_ES�� slope ���� ������ ���༭..

				if (_total_cost <= _cost_limit) {
					flag = false;
					break;
				}
				//�ش� ES�� 1�� �������� CTS ���� ��������� �񱳰� �����ٸ�,
				//�� ES�� 2~N^ver ���� ���� �񱳸� �����Ѵ�.
				if (pwq_of_version_in_ES.empty() && !is_checked_lowest_version) {
					is_checked_lowest_version = true;
					for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
						for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
							if (_selected_ES[ch][ver] == ES) {
								pwq_of_version_in_ES.insert(make_pair(_channel_list[ch].pwq[ver], make_pair(ch, ver)));
							}
						}
					}
				}
			}
		}
	}
	//set ����ϱ�
	set_version_set(_version_set, _selected_set, _selected_ES);
}