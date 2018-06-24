from flask import Flask
app = Flask(__name__)

@app.route('/', methods = ['POST'])
def login():
	if request.method == 'POST':
		#do the login
		pass
	else:
		#show the login form
		pass


