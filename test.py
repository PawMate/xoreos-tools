#!/usr/bin/python

import os
from lxml import etree
import shutil

test_result_path = './test_result'

def ParseXml(xml_path: str):
    try:
        etree.parse(xml_path)
    except Exception as e:
        return (False, str(e))
    return (True, )

def CreateOrCleanTestResultDir():
    shutil.rmtree(test_result_path, ignore_errors=True)
    os.makedirs(test_result_path)

def newXmlName(xml_name):
    return xml_name[:-4] + 'Fixed.xml'

xml_list = ['xml_dir\\' + xml.replace('/', '\\') for xml in filter(lambda file: file.endswith('.xml'), os.listdir('./xml_dir/'))]
CreateOrCleanTestResultDir()

for xml_file in xml_list:
    os.system('Project\Debug\Project.exe ' + xml_file)

    new_xml_after_move = 'test_result' + newXmlName(xml_file)[7:]
    new_xml = newXmlName(xml_file)
    os.rename(new_xml, new_xml_after_move)

    res = ParseXml(new_xml_after_move)

    if(res[0] == False):
        print(format(xml_file).ljust(40) +"bad");
        with open(new_xml_after_move[:-9] + '.txt', 'w') as f:
            f.write(res[1])
    else:
        print(format(xml_file).ljust(40)+ "good");
    os.remove(new_xml_after_move)
