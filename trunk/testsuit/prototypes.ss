/*
=!EXPECTSTART!=
{  }
     should be {}
{ a:123 }
     should be { a:123 }
123
     should be 123
shit
     should be shit
{ a:123 }
     should be { a:123 }
6
     should be 6
ZhangSan
18
defaultName
=!EXPECTEND!=
*/

console.log(Object.prototype);
console.log("     should be {}");

Object.prototype.a = 123;
Object.prototype = 123;

console.log(Object.prototype);
console.log("     should be { a:123 }");

var a = { b:1, c:2 };
console.log(a.a);
console.log("     should be 123");
a.a = 'shit';
console.log(a.a);
console.log("     should be shit");

console.log(Object.prototype);
console.log("     should be { a:123 }");

Number.prototype.fuck = function() {
	console.log(this / 2);
};

var x = 12;

x.fuck();
console.log("     should be 6");

function Person(name, sex) {
   this.name = name;
   this.sex = sex;
};

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

Employee.prototype = new Person("defaultName", "defaultSex");
Employee.prototype.getEmployeeID = function() {
	return this.employeeID;
};

var zhang = new Employee("ZhangSan", "man", "1234");
console.log(zhang.getName()); // "ZhangSan
console.log(zhang.age);			//18
delete zhang.name;
console.log(zhang.name);		//defaultName