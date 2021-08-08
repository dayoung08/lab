#include "head.h"

void channel_initialization(channel* _channel_list, bitrate_version_set* _version_set, int _version_pop_type, int _metric_type) {
	double* channel_pop = set_gamma_pop(NUM_OF_CHANNEL, K_gamma, THETA_gamma);
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
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
		set_video_metric(&(_channel_list[ch]), _version_set, _metric_type); // ���� ����Ƽ �� ���
		set_GHz(&(_channel_list[ch]), _version_set); // processing-rate ���
		set_PWQ(&(_channel_list[ch]), _version_set); // PWQ ���
	}
}

void set_video_metric(channel* _channel, bitrate_version_set* _version_set, int _metric_type) {
	//Qin, MMsys, Quality-aware Stategies for Optimizing ABR Video Streaming QoE and Reducing Data Usage, https://dl.acm.org/doi/pdf/10.1145/3304109.3306231 �� �� �����.
	//CBR ������
	mt19937 random_generation(SEED);

	int SD = 0;
	if (_metric_type == VMAF) {
		SD = rand() % 13 + 2; // �� ������ ǥ�� ������ 2~14�� ���� ����
	}
	else if (_metric_type == PSNR || _metric_type == MOS) {
		SD = rand() % 14 + 41; //  4.1~5.4
		SD /= 10;
	} // https://doi.org/10.3390/s21061949
	else if (_metric_type == SSIM) {
		SD = rand() % 23 + 4; // 0.004~0.026 vmaf �� ������� scailing��. ������ ���...
		SD /= 1000;
	}
	bool is_finished = false;
	while (!is_finished) {
		for (int ver = 1; ver <= _version_set->version_num; ver++) {
			if (ver == 1) {
				normal_distribution<double> normal_distribution_for_vmaf(_version_set->mean[ver], SD);
				_channel->video_quality[1] = normal_distribution_for_vmaf(random_generation);
				if (_metric_type == VMAF) {
					if (_channel->video_quality[1] > 100 || _channel->video_quality[1] < 0) {
						break;
					}
				}
				else if (_metric_type == PSNR || _metric_type == MOS) {
					if (_channel->video_quality[1] > 50 || _channel->video_quality[1] < 0) {
						break;
					}
				}
				else if (_metric_type == SSIM) {
					if (_channel->video_quality[1] > 1 || _channel->video_quality[1] < 0) {
						break;
					}
				}
			}
			else if (ver >= 2 && ver <= _version_set->version_num - 1) {
				normal_distribution<double> normal_distribution_for_vmaf(_version_set->mean[ver], SD);
				_channel->video_quality[ver] = normal_distribution_for_vmaf(random_generation);
				if (_channel->video_quality[ver] <= _channel->video_quality[ver - 1] || _channel->video_quality[ver] > 100 || _channel->video_quality[ver] < 0) {
					break;
				}
				if (_metric_type == VMAF) {
					if (_channel->video_quality[ver] <= _channel->video_quality[ver - 1] || _channel->video_quality[ver] > 100 || _channel->video_quality[ver] < 0) {
						break;
					}
				}
				else if (_metric_type == PSNR || _metric_type == MOS) {
					if (_channel->video_quality[ver] <= _channel->video_quality[ver - 1] || _channel->video_quality[ver] > 50 || _channel->video_quality[ver] < 0) {
						break;
					}
				}
				else if (_metric_type == SSIM) {
					if (_channel->video_quality[ver] <= _channel->video_quality[ver - 1] || _channel->video_quality[ver] > 1 || _channel->video_quality[ver] < 0) {
						break;
					}
				}
			}
			else if (ver == _version_set->version_num) {
				if (_metric_type == VMAF) {
					_channel->video_quality[_version_set->version_num] = 100;
				}
				else if (_metric_type == PSNR || _metric_type == MOS) { // https://stackoverflow.com/questions/39615894/handling-infinite-value-of-psnr-when-calculating-psnr-value-of-video
					_channel->video_quality[_version_set->version_num] = 43; // https://dl.acm.org/doi/pdf/10.1145/3304109.3306231 �� �� �����
				}
				else if (_metric_type == SSIM) {
					_channel->video_quality[_version_set->version_num] = 1;
				}
				is_finished = true;
			}

			if (_metric_type == MOS) { // An ANFIS-based Hybrid Video Quality Prediction Model for Video Streaming over Wireless Networks
				float crt = _channel->video_quality[ver];
				if (crt > 37) {
					_channel->video_quality[ver] = 5.0;
				}
				else if (crt > 31) {
					_channel->video_quality[ver] = 4.0;
				}
				else if (crt > 25) {
					_channel->video_quality[ver] = 3.0;
				}
				else if (crt > 20) {
					_channel->video_quality[ver] = 2.0;
				}
				else {
					_channel->video_quality[ver] = 1.0;
				}
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

	for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
		if (_version_set->resolution[ver] == 38402160) { //�����ϸ��ؼ� ����
			a[ver] = 2.597554426;
			b[ver] = 0.184597645;
		}
		else if (_version_set->resolution[ver] == 25601440) { //�����ϸ��ؼ� ����
			a[ver] = 1.819367444;
			b[ver] = 0.10754013;
		}
		else if (_version_set->resolution[ver] == 19201080) {
			a[ver] = 1.547002;
			b[ver] = 0.08057;
		}
		else if (_version_set->resolution[ver] == 1280720) {
			a[ver] = 1.341512;
			b[ver] = 0.060222;
		}
		else if (_version_set->resolution[ver] == 960540) { //�����ϸ��ؼ� ����
			a[ver] = 1.041912;
			b[ver] = 0.044521;
		}
		else if (_version_set->resolution[ver] == 854480) { //�����ϸ��ؼ� ����
			a[ver] = 0.961305333;
			b[ver] = 0.040296683;
		}
		else if (_version_set->resolution[ver] == 720480) { //�����ϸ��ؼ� ����
			a[ver] = 0.913512;
			b[ver] = 0.037792;
		}
		else if (_version_set->resolution[ver] == 640480) { //�����ϸ��ؼ� ����
			a[ver] = 0.884978667;
			b[ver] = 0.036296667;
		}
		else if (_version_set->resolution[ver] == 640360) {
			a[ver] = 0.827912;
			b[ver] = 0.033306;
		}
		else if (_version_set->resolution[ver] == 512384) { //�����ϸ��ؼ� ����
			a[ver] = 0.79075496;
			b[ver] = 0.03122664;
		}
		else if (_version_set->resolution[ver] == 480270) { //�����ϸ��ؼ� ����
			a[ver] = 0.717074239;
			b[ver] = 0.027103364;
		}
		else if (_version_set->resolution[ver] == 384288) { //�����ϸ��ؼ� ����
			a[ver] = 0.696173404;
			b[ver] = 0.025933724;
		}
		else if (_version_set->resolution[ver] == 426240) { //�����ϸ��ؼ� ����
			a[ver] = 0.686989703;
			b[ver] = 0.025419791;
		}
		else if (_version_set->resolution[ver] == 400224) {
			a[ver] = 0.673091;
			b[ver] = 0.024642;
		}
		else if (_version_set->resolution[ver] == 320240) { //�����ϸ��ؼ� ����
			a[ver] = 0.659016364;
			b[ver] = 0.023854364;
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

		//_channel->sum_of_transfer_data_size[set] += _version_set->data_size[1]; //���� ���������� ������ ��
		//_channel->sum_of_transfer_data_size[set] += _version_set->data_size[_version_set->version_num]; //����

		int prev_ver = 1; // set�� �ش� ������ ������ �� �Ʒ� �ִ� ������ ��Ʈ�����ϹǷ�
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { //set�� �ش� ������ �ִ� ���

			// 16 >> ((info->version_num - 2) curr_ver) : curr_ver�� 5~1�̹Ƿ�, �� 10000�� 0~4�� ��ŭ >> ������ ����Ʈ
			// �� (1)00001(1) (1)00010(1) (1)00100(1) (1)01000(1) (1)10000(1) ��
				_channel->sum_of_video_quality[set] += _channel->video_quality[ver];
				_channel->sum_of_pwq[set] += _channel->popularity[ver] * _channel->video_quality[ver];
				_channel->sum_of_version_set_GHz[set] += _channel->video_GHz[ver];
				//_channel->sum_of_transfer_data_size[set] += _version_set->data_size[ver];

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