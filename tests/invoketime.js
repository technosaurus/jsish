/*
=!EXPECTSTART!=
2
=!EXPECTEND!=
*/

// ����һ������ - add
function add(a, b) {
   add.invokeTimes++;
   return a + b;
};

// ��Ϊ��������Ҳ�Ƕ�������Ϊ����add����һ�����ԣ�������¼�˺��������õĴ���
add.invokeTimes = 0;
add(1 + 1);
add(2 + 3);
puts(add.invokeTimes); // 2
