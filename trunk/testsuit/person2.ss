/*
=!EXPECTSTART!=
ZhangSan
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
	   
	function Employee(name, sex, employeeID) {
	    this.name = name;
	    this.sex = sex;
	    this.employeeID = employeeID;
	};
	
	// 将Employee的原型指向Person的一个实例
	// 因为Person的实例可以调用Person原型中的方法, 所以Employee的实例也可以调用Person原型中的所有属性。
	Employee.prototype = new Person();
	Employee.prototype.getEmployeeID = function() {
	    return this.employeeID;
	};
	var zhang = new Employee("ZhangSan", "man", "1234");
	console.log(zhang.getName()); // "ZhangSan
