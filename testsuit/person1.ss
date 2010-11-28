/*
=!EXPECTSTART!=
ZhangSan
ChunHua
18
20
18
=!EXPECTEND!=
*/

	// 构造函数
	   function Person(name, sex) {
	       this.name = name;
	       this.sex = sex;
	   };
	   // 定义Person的原型，原型中的属性可以被自定义对象引用
	   Person.prototype = {
	       getName: function() {
	           return this.name;
	       },
	       getSex: function() {
	           return this.sex;
	       },
		   age: 18
	   };
	var zhang = new Person("ZhangSan", "man");
	console.log(zhang.getName()); // "ZhangSan"
	var chun = new Person("ChunHua", "woman");
	console.log(chun.getName()); // "ChunHua"
	
	console.log(zhang.age);		//18
	zhang.age = 20;
	console.log(zhang.age);		//20
	delete zhang.age;
	console.log(zhang.age);
