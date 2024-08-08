import argparse
from flask import Flask, jsonify, request
import json
import sys
import time
import traceback
sys.path.append('../engine')
from fm_engine.engine import FmIndexEngine

parser = argparse.ArgumentParser()
parser.add_argument('--MODE', type=str, default='api', choices=['api', 'dev', 'demo'])
parser.add_argument('--FLASK_PORT', type=int, default=5000)
parser.add_argument('--CONFIG_FILE', type=str, default='api_config.json')
parser.add_argument('--LOG_PATH', type=str, default=None)
args = parser.parse_args()

class Processor:

    def __init__(self, config):
        assert 'dir' in config

        self.engine = FmIndexEngine(index_dir=config['dir'])

    def process(self, query_type, query, **kwargs):
        if type(query) != str:
            return {'error': f'query must be a string!'}

        start_time = time.time()
        result = getattr(self, query_type)(query, **kwargs)
        end_time = time.time()
        result['latency'] = (end_time - start_time) * 1000

        return result

    def count(self, query):
        return self.engine.count(query=query)

    def reconstruct(self, query, num_occ):
        result = self.engine.reconstruct(query=query, num_occ=num_occ)

        if 'error' in result:
            return result

        spans = [(result['text'], None)]
        if len(query) > 0:
            needle = query
            new_spans = []
            for span in spans:
                if span[1] is not None:
                    new_spans.append(span)
                else:
                    haystack = span[0]
                    new_spans += self._replace(haystack, needle, label='0')
            spans = new_spans
        result = {'spans': spans}

        return result

    def _replace(self, haystack, needle, label):
        spans = []
        while True:
            pos = -1
            for p in range(len(haystack) - len(needle) + 1):
                if haystack[p:p+len(needle)] == needle:
                    pos = p
                    break
            if pos == -1:
                break
            if pos > 0:
                spans.append((haystack[:pos], None))
            spans.append((haystack[pos:pos+len(needle)], label))
            haystack = haystack[pos+len(needle):]
        if len(haystack) > 0:
            spans.append((haystack, None))
        return spans

PROCESSOR_BY_INDEX = {}
with open(args.CONFIG_FILE) as f:
    configs = json.load(f)
    for config in configs:
        PROCESSOR_BY_INDEX[config['name']] = Processor(config)

# save log under home directory
if args.LOG_PATH is None:
    args.LOG_PATH = f'/home/ubuntu/logs/flask_{args.MODE}.log'
log = open(args.LOG_PATH, 'a')
app = Flask(__name__)

@app.route('/', methods=['POST'])
def query():
    data = request.json
    print(data)
    log.write(json.dumps(data) + '\n')
    log.flush()

    try:
        query_type = data['query_type']
        index = data['index']
        query = data['query']
        for key in ['query_type', 'index', 'query', 'source', 'timestamp']:
            if key in data:
                del data[key]
    except KeyError as e:
        return jsonify({'error': f'[Flask] Missing required field: {e}'}), 400

    try:
        processor = PROCESSOR_BY_INDEX[index]
    except KeyError:
        return jsonify({'error': f'[Flask] Invalid index: {index}'}), 400
    if not hasattr(processor.engine, query_type):
        return jsonify({'error': f'[Flask] Invalid query_type: {query_type}'}), 400

    try:
        result = processor.process(query_type, query, **data)
    except Exception as e:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        traceback.print_exception(exc_type, exc_value, exc_traceback)
        return jsonify({'error': f'[Flask] Internal server error: {e}'}), 500
    return jsonify(result), 200

app.run(host='0.0.0.0', port=args.FLASK_PORT, threaded=False)
