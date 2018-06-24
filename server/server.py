from flask import Flask
from flask import request
app = Flask(__name__)

@app.route('/', methods = ['GET', 'POST'])
def login():
    error = None
    if request.method == 'POST':
        print(' The post request. ')
        '''
        if valid_login(request.form['username'],
                       request.form['password']):
            return log_the_user_in(request.form['username'])
        else:
            error = 'Invalid username/password'
        '''
    else:
        print('Other request.')
    return 'login finished.'

if __name__ == '__main__':
	app.run()
