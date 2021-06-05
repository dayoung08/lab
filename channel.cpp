#include "head.h"

void channel_initialization(channel* _channel_list, bitrate_version_set* _version_set, int _version_pop_type) {
	double* channel_pop = set_gamma_pop(CHANNEL_NUM, K_gamma, THETA_gamma);
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		_channel_list[ch].index = ch;
		_channel_list[ch].video_quality = (double*)malloc(sizeof(double) * (_version_set->version_num + 1));
		_channel_list[ch].popularity = (double*)malloc(sizeof(double) * (_version_set->version_num + 1));
		_channel_list[ch].video_GHz = (double*)malloc(sizeof(double) * (_version_set->version_num + 1));
		_channel_list[ch].pwq = (double*)malloc(sizeof(double) * (_version_set->version_num + 1));

		_channel_list[ch].sum_of_video_quality = (double*)malloc(sizeof(double) * (_version_set->version_set_num + 1));
		_channel_list[ch].sum_of_pwq = (double*)malloc(sizeof(double) * (_version_set->version_set_num + 1));
		_channel_list[ch].sum_of_version_set_GHz = (double*)malloc(sizeof(double) * (_version_set->version_set_num + 1));
		_channel_list[ch].sum_of_transfer_data_size = (double*)malloc(sizeof(double) * (_version_set->version_set_num + 1));

		_channel_list[ch].version_pop_type = _version_pop_type;
		double* ver_pop = set_version_pop(_version_set, _channel_list[ch].version_pop_type);
		
		_channel_list[ch].popularity[0] = channel_pop[ch];
		for (int ver = 1; ver <= _version_set->version_num; ver++) {
			_channel_list[ch].video_quality[ver] = 0;
			_channel_list[ch].popularity[ver] = channel_pop[ch] * ver_pop[ver];
			_channel_list[ch].video_GHz[ver] = 0;
			_channel_list[ch].pwq[ver] = 0;
		}
		for (int set = 1; set <= _version_set->version_set_num; set++) {
			_channel_list[ch].sum_of_video_quality[set] = 0;
			_channel_list[ch].sum_of_pwq[set] = 0;
			_channel_list[ch].sum_of_version_set_GHz[set] = 0;
			_channel_list[ch].sum_of_transfer_data_size[set] = 0;
		}
		//�� ���� �α⵵ ���
		set_VMAF(&(_channel_list[ch]), _version_set); // ���� ����Ƽ �� ���
		set_GHz(&(_channel_list[ch]), _version_set); // processing-rate ���
		set_PWQ(&(_channel_list[ch]), _version_set); // PWQ ���
	}
}

void set_metric_score(channel* _channel_list, bitrate_version_set* _version_set, int metric_type) {
	//VMAF�� Qin, MMsys, Quality-aware Stategies for Optimizing ABR Video Streaming QoE and Reducing Data Usage, https://dl.acm.org/doi/pdf/10.1145/3304109.3306231 �� �� �����.
	//CBR ������
	mt19937 random_generation(SEED);
	double* mean = (double*)malloc(sizeof(double) * (_version_set->version_num + 1)); // bitrate set�� ���� VMAF mean�� �޶���. 
	if (_version_set->index == 0) { // zencoder ����
		if (metric_type == VMAF) {
			mean[6] = 95; //2000 
			mean[5] = 92.5; //1500
			mean[4] = 90; //1000
			mean[3] = 72; //600
			mean[2] = 60; //400
			mean[1] = 40; //200
		}
		else if (metric_type == PSNR || metric_type == MOS) { // https://dl.acm.org/doi/pdf/10.1145/3304109.3306231 �� �� �����
			mean[6] = 41.5; //2000 
			mean[5] = 40; //1500
			mean[4] = 38.5; //1000
			mean[3] = 35; //600
			mean[2] = 33.5; //400
			mean[1] = 32; //200
		}
		else if (metric_type == SSIM) { //SSIM->MOS ��ȯ
			//Light-weight Video Coding Based on Perceptual Video Quality for Live Streaming https://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=8603274 
			mean[6] = 0.97; //2000 
			mean[5] = 0.96; //1500
			mean[4] = 0.95; //1000
			mean[3] = 0.93; //600
			mean[2] = 0.91; //400
			mean[1] = 0.87; //200
		}
	}
	//zencoder ����� �� vmaf�� ���Ƿ� ó������ �ʾ���.
	else if (_version_set->index == 1) { // youtube
		mean[10] = 99.9; //23500
		mean[9] = 99.5; //13500
		mean[8] = 98.6; //9500
		mean[7] = 98.4; //6750
		mean[6] = 98.2; //4500;
		mean[5] = 98; //4125;
		mean[4] = 96; //2750;
		mean[3] = 91.3; //1250;
		mean[2] = 77; //700;
		mean[1] = 65; //500;
	}
	else if (_version_set->index == 2) { //netflix
		mean[9] = 98.1; //4300
		mean[8] = 97; //3000
		mean[7] = 95.5; //2350
		mean[6] = 93.8; // 1750
		mean[5] = 90.2; //1050
		mean[4] = 80; //750
		mean[3] = 67.5; //560
		mean[2] = 57.5; //375
		mean[1] = 43; // 235
	}
	else if (_version_set->index == 3) {// IBM Watson Media
		mean[6] = 98.3; //6000
		mean[5] = 96; // 2750
		mean[4] = 91.5; // 1350
		mean[3] = 91.5; // 1350
		mean[2] = 90; //1000
		mean[1] = 60; //400
	}
	else if (_version_set->index == 4) { // �� https://doi.org/10.1145/3123266.3123426
		mean[8] = 98.3; //6000
		mean[7] = 97.8; //4000
		mean[6] = 97; // 3000
		mean[5] = 95.5; //2400
		mean[4] = 92.5; //1500
		mean[3] = 82; //807
		mean[2] = 65; //505
		mean[1] = 45; // 253
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//normal_distribution<double> normal_distribution_for_metric[7]; //�̰� ��� ���� �迭�� �ٲ��� ��������
		/*normal_distribution<double> normal_distribution_for_metric_ver1;
		normal_distribution<double> normal_distribution_for_metric_ver2;
		normal_distribution<double> normal_distribution_for_metric_ver3;
		normal_distribution<double> normal_distribution_for_metric_ver4;
		normal_distribution<double> normal_distribution_for_metric_ver5;
		normal_distribution<double> normal_distribution_for_metric_ver6;*/
		//ver 7�� vmaf 100��
		double SD;
		if (metric_type == VMAF) {
			SD = rand() % 13 + 2; // �� ������ ǥ�� ������ 2~14�� ���� ����
		}
		else if (metric_type == PSNR || metric_type == MOS) {
			SD = rand() % 14 + 41; //  4.1~5.4
			SD /= 10;
		} // https://doi.org/10.3390/s21061949
		else if (metric_type == SSIM) {
			SD = rand() % 23 + 4; // 0.004~0.026 vmaf �� ������� scailing��. ������ ���...
			SD /= 1000;
		}
		_channel_list[1];

		for (int ver = 1; ver <= _version_set->version_num; ver++) {
			normal_distribution<double> normal_distribution_for_metric(mean[ver], SD);
			if (ver == 1) {
				_channel_list[ch].video_quality[1] = normal_distribution_for_metric(random_generation);
				if (metric_type == VMAF) {
					while (_channel_list[ch].video_quality[1] > 100 || _channel_list[ch].video_quality[1] < 0) {
						_channel_list[ch].video_quality[1] = normal_distribution_for_metric(random_generation);
					}
				}
				else if (metric_type == PSNR || metric_type == MOS) {
					while (_channel_list[ch].video_quality[1] > 50 || _channel_list[ch].video_quality[1] < 0) {
						_channel_list[ch].video_quality[1] = normal_distribution_for_metric(random_generation);
					}
				}
				else if (metric_type == SSIM) {
					while (_channel_list[ch].video_quality[1] > 1 || _channel_list[ch].video_quality[1] < 0) {
						_channel_list[ch].video_quality[1] = normal_distribution_for_metric(random_generation);
					}
				}
			}
			else if (ver >= 2 && ver <= _version_set->version_num - 1) {
				_channel_list[ch].video_quality[ver] = normal_distribution_for_metric(random_generation);
				if (metric_type == VMAF) {
					while (_channel_list[ch].video_quality[ver] <= _channel_list[ch].video_quality[ver - 1] || _channel_list[ch].video_quality[ver] > 100 || _channel_list[ch].video_quality[ver] < 0) {
						_channel_list[ch].video_quality[ver] = normal_distribution_for_metric(random_generation);
					}
				}
				else if (metric_type == PSNR || metric_type == MOS) {
					while (_channel_list[ch].video_quality[ver] <= _channel_list[ch].video_quality[ver - 1] || _channel_list[ch].video_quality[ver] > 50 || _channel_list[ch].video_quality[ver] < 0) {
						_channel_list[ch].video_quality[ver] = normal_distribution_for_metric(random_generation);
					}
				}
				else if (metric_type == SSIM) {
					while (_channel_list[ch].video_quality[ver] <= _channel_list[ch].video_quality[ver - 1] || _channel_list[ch].video_quality[ver] > 1 || _channel_list[ch].video_quality[ver] < 0) {
						_channel_list[ch].video_quality[ver] = normal_distribution_for_metric(random_generation);
					}
				}
			}
			else if (ver == _version_set->version_num) {
				if (metric_type == VMAF) {
					_channel_list[ch].video_quality[_version_set->version_num] = 100;
				}
				else if (metric_type == PSNR || metric_type == MOS) { // https://stackoverflow.com/questions/39615894/handling-infinite-value-of-psnr-when-calculating-psnr-value-of-video
					_channel_list[ch].video_quality[_version_set->version_num] = 43; // https://dl.acm.org/doi/pdf/10.1145/3304109.3306231 �� �� �����
				}
				else if (metric_type == SSIM) {
					_channel_list[ch].video_quality[_version_set->version_num] = 1;
				}
			}
		}
	}
	if (metric_type == MOS) { // An ANFIS-based Hybrid Video Quality Prediction Model for Video Streaming over Wireless Networks
		for (int channel = 1; channel <= CHANNEL_NUM; channel++) {
			for (int ver = 1; ver <= _version_set->version_num; ver++) {
				double crt = _channel_list[channel].video_quality[ver];
				if (crt > 37) {
					_channel_list[channel].video_quality[ver] = 5.0;
				}
				else if (crt > 31) {
					_channel_list[channel].video_quality[ver] = 4.0;
				}
				else if (crt > 25) {
					_channel_list[channel].video_quality[ver] = 3.0;
				}
				else if (crt > 20) {
					_channel_list[channel].video_quality[ver] = 2.0;
				}
				else {
					_channel_list[channel].video_quality[ver] = 1.0;
				}
			}
		}
	}
}


void set_VMAF(channel* _channel, bitrate_version_set* _version_set) {
	//Qin, MMsys, Quality-aware Stategies for Optimizing ABR Video Streaming QoE and Reducing Data Usage, https://dl.acm.org/doi/pdf/10.1145/3304109.3306231 �� �� �����.
	//CBR ������
	mt19937 random_generation(SEED);

	int* mean = (int*)malloc(sizeof(int) * (_version_set->version_num + 1)); // bitrate set�� ���� VMAF mean�� �޶���. 
	mean[6] = 95; //2000 
	mean[5] = 92.5; //1500
	mean[4] = 90; //1000
	mean[3] = 72; //600
	mean[2] = 60; //400
	mean[1] = 40; //200

	int SD = rand() % 13 + 2; // �� ������ ǥ�� ������ 2~14�� ���� ����
	bool is_finished = false;
	while (!is_finished) {
		for (int ver = 1; ver <= _version_set->version_num; ver++) {
			normal_distribution<double> normal_distribution_for_vmaf(mean[ver], SD);
			if (ver == 1) {
				_channel->video_quality[1] = normal_distribution_for_vmaf(random_generation);
				if (_channel->video_quality[1] > 100 || _channel->video_quality[1] < 0) {
					break;
				}
			}
			else if (ver >= 2 && ver <= _version_set->version_num - 1) {
				_channel->video_quality[ver] = normal_distribution_for_vmaf(random_generation);
				if (_channel->video_quality[ver] <= _channel->video_quality[ver - 1] || _channel->video_quality[ver] > 100 || _channel->video_quality[ver] < 0) {
					break;
				}
			}
			else if (ver == _version_set->version_num) {
				_channel->video_quality[_version_set->version_num] = 100;
				is_finished = true;
			}
		}
	}
}

void set_GHz(channel* _channel, bitrate_version_set* _version_set) {
	//ä�� ���� GHz�� �� �ٸ��� �ຸ��.
	//MMsys Aparicio-Pardo�� Transcoding Live Adaptive Video Streams at a Massive Scale in the Cloud
	//CPU cycle ������ ���� GHz ����� ����� ���̺�� �÷� ��. zecoder bitrate set ���յ� ���⿡ ����.
	double* a = (double*)malloc(sizeof(double) * (_version_set->version_num));
	double* b = (double*)malloc(sizeof(double) * (_version_set->version_num)); // �� �� ������ Ʈ�����ڵ� �������� ���Ƿ� ���� �Ϳ� ������ ��
	//int r[] = { 0, 200, 400, 600, 1000, 1500, 2000 };

	mt19937 random_generation(SEED);
	for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
		if (_version_set->resolution[ver] == 19201080) {
			a[ver] = 1.547002;
			b[ver] = 0.08057;
		}
		else if (_version_set->resolution[ver] == 1280720) {
			a[ver] = 1.341512;
			b[ver] = 0.060222;
		}
		else if (_version_set->resolution[ver] == 640360) {
			a[ver] = 0.827912;
			b[ver] = 0.033306;
		}
		else if (_version_set->resolution[ver] == 400224) {
			a[ver] = 0.673091;
			b[ver] = 0.024642;
		}

		double GHz = a[ver] * pow((double)_version_set->bitrate[ver]/1000, b[ver]);
		//double GHz = a[version] + pow(r[version], b[version]);
		GHz += ((GHz * ((double)(rand() % 1001) / 10000)) - (GHz * 0.05)); // += 10% ������ �ؼ� GHz����.
		GHz *= 4; // ���� 4 core ��. perf�� cycle ���� ���� �� �ھ��� �����. 
		/*if (GHz <= 0) {
			cout << "����";
		}*/
		/*if (GHz <= 0) {
			cout << "����";
		}*/
		_channel->video_GHz[ver] = GHz;
	}
}

void set_PWQ(channel* _channel, bitrate_version_set* _version_set) {
	//number_for_bit_opration = pow(2, info->version_num - 3);
	//set_versions_number = info->version_num - 2;
	srand(SEED);

	_channel->pwq[1] = _channel->popularity[1] * _channel->video_quality[1]; //�⺻������ ���� ���� ������ �ݵ�� ���ԵǹǷ�
	_channel->pwq[_version_set->version_num] = _channel->popularity[_version_set->version_num] * _channel->video_quality[_version_set->version_num]; //�⺻������ ���������� �ݵ�� ���ԵǹǷ�

	for (int ver = 1; ver <= _version_set->version_num; ver++) {
		_channel->pwq[ver] = _channel->popularity[ver] * _channel->video_quality[ver];
	}

	for (int set = 1; set <= _version_set->version_set_num; set++) { //�ҽ��� ���� 1080p
		_channel->sum_of_video_quality[set] += _channel->video_quality[1];//�⺻������ ���� ���� ������ �ݵ�� ���ԵǹǷ�
		_channel->sum_of_video_quality[set] += _channel->video_quality[_version_set->version_num];//�⺻������ ���������� �ݵ�� ���ԵǹǷ�

		_channel->sum_of_pwq[set] += _channel->popularity[1] * _channel->video_quality[1]; //�⺻������ ���� ���� ������ �ݵ�� ���ԵǹǷ�
		_channel->sum_of_pwq[set] += _channel->popularity[_version_set->version_num] * _channel->video_quality[_version_set->version_num]; //�⺻������ ���������� �ݵ�� ���ԵǹǷ�

		_channel->sum_of_version_set_GHz[set] += _channel->video_GHz[1]; //�⺻������ ���� ���� ������ �ݵ�� Ʈ�����ڵ� �ϹǷ�

		_channel->sum_of_transfer_data_size[set] += _version_set->data_size[1]; //���� ���������� ������ ��
		_channel->sum_of_transfer_data_size[set] += _version_set->data_size[_version_set->version_num]; //����

		int prev_ver = 1; // set�� �ش� ������ ������ �� �Ʒ� �ִ� ������ ��Ʈ�����ϹǷ�
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { //set�� �ش� ������ �ִ� ���

			// 16 >> ((info->version_num - 2) curr_ver) : curr_ver�� 5~1�̹Ƿ�, �� 10000�� 0~4�� ��ŭ >> ������ ����Ʈ
			// �� (1)00001(1) (1)00010(1) (1)00100(1) (1)01000(1) (1)10000(1) ��
				_channel->sum_of_video_quality[set] += _channel->video_quality[ver];
				_channel->sum_of_pwq[set] += _channel->popularity[ver] * _channel->video_quality[ver];
				_channel->sum_of_version_set_GHz[set] += _channel->video_GHz[ver];
				_channel->sum_of_transfer_data_size[set] += _version_set->data_size[ver];

				prev_ver = ver;
			}
			else {  //set�� �ش� ������ ���� ���
				_channel->sum_of_video_quality[set] += _channel->video_quality[prev_ver];
				_channel->sum_of_pwq[set] += _channel->popularity[ver] * _channel->video_quality[prev_ver];
			}
		}
	}
}

//https://en.wikipedia.org/wiki/Gamma_distribution
double* set_gamma_pop(int length, double k, double theta) {
	double sum = 0;
	double* gamma_pdf = (double*)malloc(sizeof(double) * (length + 1));
	for (int value = 1; value <= length; value++) {
		double result = (pow(value, (k - 1)) * std::pow(M_E, (-value / theta))) / (tgamma(k) * pow(theta, k));
		gamma_pdf[value] = result;
		sum += result;
	}

	for (int value = 1; value <= length; value++) {
		gamma_pdf[value] /= sum;
		//cout << gamma_pdf[value] << " ";
	}
	//ä�� ���� ���̱Ⱑ ����ϱ� ��� ������ �ε� ��������
	return gamma_pdf;
}

double* set_version_pop(bitrate_version_set* __version_set, int _version_pop_type) {
	//Caching Strategies in Transcoding-enabled Proxy Systems for Streaming Media Distribution Networks
	//https://www.hpl.hp.com/techreports/2003/HPL-2003-261.pdf �����
	double* ver_pop = (double*)malloc(sizeof(double) * (__version_set->version_num + 1));
	double mean;

	if (_version_pop_type == HVP) {
		mean = __version_set->version_num;
	}
	else if (_version_pop_type == MVP) {
		mean = (1 + __version_set->version_num) / 2;
	}
	else if (_version_pop_type == LVP) {
		mean = 1;
	}
	else if (_version_pop_type == RVP) {
		mean = (rand() % __version_set->version_num) + 1;
	}

	double SD = SIGMA;
	// ��� ���� ������ SD ���� ���� fix������ �ϰ�, ���� ���� �޶������� �ùķ��̼��� ���� �ϰ� �ִ�.
	// ���� �̰� �׳� ���迡 �߰� �ϴ°� ���ڴ�.
	SD += ((SD * ((double)(rand() % 1001) / 10000)) - (SD * 0.05)); // += 5% ������ �ؼ� SD����.

	//ǥ�������� ���� ������ �� �ڷ�� �����ϴ�. ���� ��� �� �ڷ��� ���� ������ ��Ƽ�����̸� ǥ�������� ���� ������ ��Ƽ�����Դϴ�.

	//�� ������ ���� ���� �츮 �Ϳ� �Ҹ�. �ֳ��ϸ� ���� ���� mean ������ �α⵵�� �����Ǳ� ����.
	//A small �� indicates that most of the accesses are to one version(m) and a large �� indicates that the accesses to different version are evenly distributed.
	//���� SD�� ��κ��� �׼����� �� ����(m)�� ���� ���̰� ū SD�� �ٸ� ������ ���� �׼����� �յ��ϰ� �л�Ǿ� ������ ��Ÿ���ϴ�.

	double sum = 0;
	for (int ver = 1; ver <= __version_set->version_num; ver++) {
		double result = 0;
		result = (1 / (sqrt(2 * PI) * SD)) * pow(M_E, -pow((ver - mean), 2) / (2 * pow(SD, 2)));

		ver_pop[ver] = result;
		sum += result;
	}
	for (int ver = 1; ver <= __version_set->version_num; ver++) {
		ver_pop[ver] /= sum;
	}
	//��� ���� ���� 1�̾����

	return ver_pop;
}

double channel::get_channel_popularity() {
	return popularity[0];
}