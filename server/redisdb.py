import redis

class RedisDB:
	def __init__(self, servaddr = 'localhost', port = 6379, db = 0):
		self.host = servaddr
		self.port = port
		self.db = db
		self.hRedisDB = redis.StrictRedis(host = self.host, port = self.port, db = self.db)

	def do_user_login(self, username, password):
		if self.check_user_info(username, password):
			return self.user_login(username)
		else:
			return False

	def check_user_info(self, username, password):
		if password == str(self.hRedisDB.get(username), encoding='utf-8'):
			return True
		else:
			return False

	def user_login(self, username):
		print('user: ' + username + '  login !')
		return True

db = RedisDB()

if db.do_user_login('testun', 'testpd'):
	print('finished login!!!')
else:
	print('exit!!!')