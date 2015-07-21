###SIM SELECT命令应答信息分析

文件： ETSI_TS_102_221_V9.0.0.pdf

参考：11.1.1.3  Response Data这一章节分析应答信息数据如下

	C0		INS
	62		Length of FCP template
	21  	file description len
	
	82 		Tag
	05 		length
	42 		File descriptor byte
	21 		Data coding byte
	0020 	Record length
	04		Number of records
	
	83026F40A506C00100DE01008A01058B036F0605800200808800
	
	9000	结束符