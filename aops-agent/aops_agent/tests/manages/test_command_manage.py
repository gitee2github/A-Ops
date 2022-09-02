#!/usr/bin/python3
# ******************************************************************************
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
# licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN 'AS IS' BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.
# ******************************************************************************/
import unittest
import warnings
from unittest import mock

import responses

from aops_agent.conf.status import SUCCESS, PARAM_ERROR
from aops_agent.manages.command_manage import Command
from aops_agent.models.custom_exception import InputError


class TestRegister(unittest.TestCase):

    def setUp(self) -> None:
        warnings.simplefilter('ignore', ResourceWarning)

    @responses.activate
    def test_register_should_return_200_when_input_correct(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111",
            "agent_port": "12000"
        }
        responses.add(responses.POST,
                      'http://127.0.0.1:11111/manage/host/add',
                      json={"token": "hdahdahiudahud", "code": SUCCESS},
                      status=SUCCESS,
                      content_type='application/json'
                      )
        data = Command.register(input_data)
        self.assertEqual(SUCCESS, data)

    def test_register_should_return_param_error_when_input_web_username_is_null(self):
        input_data = {
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_web_username_is_not_string(self):
        input_data = {
            "web_username": 12345,
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_web_password_is_null(self):
        input_data = {
            "web_username": "admin",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_web_password_is_not_string(self):
        input_data = {
            "web_username": "admin",
            "web_password": 123456,
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_host_name_is_null(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_host_name_is_not_string(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": 12345,
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_host_group_name_is_null(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_host_group_name_is_not_string(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": True,
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_management_is_null(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_management_is_not_boolean(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": "string",
            "manager_ip": "127.0.0.1",
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_manager_ip_is_null(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_manager_ip_is_not_string(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_port": "11111"
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_manager_port_is_null(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_manager_port_is_not_string(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": 80
        }
        data = Command.register(input_data)
        self.assertEqual(PARAM_ERROR, data)

    def test_register_should_return_param_error_when_input_agent_port_is_not_string(self):
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111",
            "agent_port": 11000
        }
        data = Command.register(input_data)
        self.assertEqual(data, PARAM_ERROR)

    @responses.activate
    def test_register_should_return_success_when_input_with_no_agent_port(self):
        responses.add(responses.POST,
                      'http://127.0.0.1:11111/manage/host/add',
                      json={"token": "hdahdahiudahud", "code": SUCCESS},
                      status=SUCCESS,
                      content_type='application/json'
                      )
        input_data = {
            "web_username": "admin",
            "web_password": "changeme",
            "host_name": "host01",
            "host_group_name": "2333",
            "management": False,
            "manager_ip": "127.0.0.1",
            "manager_port": "11111",
        }
        data = Command.register(input_data)
        self.assertEqual(SUCCESS, data)

    @mock.patch('aops_agent.manages.command_manage.get_shell_data')
    def test_get_memory_info_should_return_memory_info_when_get_shell_data_is_correct(self, mock_shell_data):
        mock_shell_data.return_value = """
            Memory Device
                    Array Handle: 0x0006
                    Error Information Handle: Not Provided
                    Total Width: 72 bits
                    Data Width: 64 bits
                    Size: 16 GB
                    Form Factor: DIMM
                    Set: None
                    Locator: DIMM170 J31
                    Bank Locator: SOCKET 1 CHANNEL 7 DIMM 0
                    Type: DDR4
                    Type Detail: Synchronous Registered (Buffered)
                    Speed: 2000 MT/s
                    Manufacturer: Test1
                    Serial Number: 129C7699
                    Asset Tag: 1939
                    Part Number: HMA82GR7CJR4N-WM
            Memory Device
                    Form Factor: DIMM
                    Set: None
                    Size: 32 GB
                    Locator: DIMM170 J31
                    Bank Locator: SOCKET 1 CHANNEL 7 DIMM 0
                    Type: DDR4
                    Type Detail: Synchronous Registered (Buffered)
                    Speed: 2000 MT/s
                    Manufacturer: Test2
            """
        expect_res = {
            'total': 2,
            'info': [
                {'size': '16 GB',
                 'type': 'DDR4',
                 'speed': '2000 MT/s',
                 'manufacturer': 'Test1'
                 },
                {'size': '32 GB',
                 'type': 'DDR4',
                 'speed': '2000 MT/s',
                 'manufacturer': 'Test2'
                 }
            ]
        }

        res = Command()._Command__get_memory_info()
        self.assertEqual(expect_res, res)

    @mock.patch('aops_agent.manages.command_manage.get_shell_data')
    def test_get_memory_info_should_return_empty_list_when_memory_info_is_not_showed(self, mock_shell_data):

        mock_shell_data.return_value = """
                    Memory Device
                    Array Handle: 0x0006
                    Error Information Handle: Not Provided
                    Total Width: Unknown
                    Data Width: Unknown
                    Size: No Module Installed
                    Form Factor: DIMMis
                    Set: None
                    Locator: DIMM171 J32
                    Bank Locator: SOCKET 1 CHANNEL 7 DIMM 1
                    Type: Unknown
                    Type Detail: Unknown Synchronous
                    Speed: Unknown
        """
        expect_res = {'info': [], 'total': 0}

        res = Command()._Command__get_memory_info()
        self.assertEqual(expect_res, res)

    @mock.patch('aops_agent.manages.command_manage.get_shell_data')
    def test_get_memory_info_should_return_empty_dict_when_get_shell_data_is_incorrect_data(self, mock_shell_data):
        """
            This situation exists in the virtual machine
        """
        mock_shell_data.return_value = """
                test text 
        """
        res = Command()._Command__get_memory_info()
        self.assertEqual({}, res)

    @mock.patch('aops_agent.manages.command_manage.get_shell_data')
    def test_get_memory_info_should_return_empty_dict_when_get_shell_data_error(self, mock_shell_data):
        mock_shell_data.side_effect = InputError('')
        res = Command()._Command__get_memory_info()
        self.assertEqual({}, res)

    @mock.patch('aops_agent.manages.command_manage.get_shell_data')
    def test_get_memory_size_should_return_memory_size_when_get_shell_data_is_correct_data(self, mock_shell_data):
        mock_shell_data.return_value = '''
            Memory block size:       128M
            Total online memory:     2.5G
            Total offline memory:      0B
        '''
        res = Command._Command__get_total_online_memory()
        self.assertEqual('2.5G', res)

    @mock.patch('aops_agent.manages.command_manage.get_shell_data')
    def test_get_memory_size_should_return_empty_str_when_get_shell_data_is_incorrect_data(self, mock_shell_data):
        mock_shell_data.return_value = '''
            Memory block size:       128M
        '''
        res = Command._Command__get_total_online_memory()
        self.assertEqual('', res)

    @mock.patch('aops_agent.manages.command_manage.get_shell_data')
    def test_get_memory_size_should_return_empty_str_when_get_shell_data_error(self, mock_shell_data):
        mock_shell_data.side_effect = InputError('')
        res = Command._Command__get_total_online_memory()
        self.assertEqual('', res)